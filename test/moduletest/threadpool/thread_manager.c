/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "thread_manager.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>

#include "appspawn_utils.h"
#include "list.h"

typedef struct {
    atomic_uint threadExit;
    uint32_t index;
    pthread_t threadId;
} ThreadNode;

typedef struct {
    pthread_mutex_t mutex;        // 保护执行队列
    pthread_cond_t cond;          // 线程等待条件
    ListNode taskList;            // 任务队列，任务还没有启动
    ListNode waitingTaskQueue;    // 启动的任务，排队等待执行
    ListNode executingTaskQueue;  // 正在执行
    ListNode executorQueue;       // 执行节点，保存 TaskExecuteNode
    uint32_t executorCount;
    uint32_t maxThreadCount;
    uint32_t currTaskId;
    struct timespec lastAdjust;
    uint32_t validThreadCount;
    ThreadNode threadNode[1];  // 线程信息，控制线程的退出和结束
} ThreadManager;

typedef struct {
    uint32_t taskId;
    ListNode node;
    ListNode executorList;
    uint32_t totalTask;
    atomic_uint taskFlags;  // 表示任务是否被取消，各线程检查后决定任务线程是否结束
    atomic_uint finishTaskCount;
    const ThreadContext *context;
    TaskFinishProcessor finishProcess;
    pthread_mutex_t mutex;  // 保护执行队列
    pthread_cond_t cond;    // 同步执行时，等待确认
} TaskNode;

typedef struct {
    ListNode node;         // 保存sub task到对应的task，方便管理
    ListNode executeNode;  // 等待处理的任务节点
    TaskNode *task;
    const ThreadContext *context;
    TaskExecutor executor;
} TaskExecuteNode;

static ThreadManager *g_threadManager = NULL;

static void *ManagerThreadProc(void *args);
static void *ThreadExecute(void *args);

static void SetCondAttr(pthread_cond_t *cond)
{
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(cond, &attr);
    pthread_condattr_destroy(&attr);
}

static void ConvertToTimespec(int time, struct timespec *tm)
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t ns = time;
    ns *= APPSPAWN_MSEC_TO_NSEC;
    ns += start.tv_sec * APPSPAWN_SEC_TO_NSEC + start.tv_nsec;
    tm->tv_sec = ns / APPSPAWN_SEC_TO_NSEC;
    tm->tv_nsec = ns % APPSPAWN_SEC_TO_NSEC;
}

static TaskExecuteNode *PopTaskExecutor(ThreadManager *mgr)
{
    TaskExecuteNode *executor = NULL;
    pthread_mutex_lock(&mgr->mutex);
    ListNode *node = mgr->executorQueue.next;
    if (node != &mgr->executorQueue) {
        OH_ListRemove(node);
        OH_ListInit(node);
        executor = ListEntry(node, TaskExecuteNode, executeNode);
        mgr->executorCount--;
    }
    pthread_mutex_unlock(&mgr->mutex);
    return executor;
}

static int AddExecutor(ThreadManager *mgr, const TaskNode *task)
{
    ListNode *node = task->executorList.next;
    while (node != &task->executorList) {
        TaskExecuteNode *executor = ListEntry(node, TaskExecuteNode, node);
        APPSPAWN_LOGV("AddExecutor task: %{public}u executorCount: %{public}u executor: %{public}u",
            task->taskId, mgr->executorCount, executor->task->taskId);

        // 插入尾部执行
        pthread_mutex_lock(&mgr->mutex);
        OH_ListRemove(&executor->executeNode);
        OH_ListInit(&executor->executeNode);
        OH_ListAddTail(&mgr->executorQueue, &executor->executeNode);
        mgr->executorCount++;
        pthread_mutex_unlock(&mgr->mutex);

        node = node->next;
    }
    return 0;
}

static void RunExecutor(ThreadManager *mgr, ThreadNode *threadNode, uint32_t maxCount)
{
    APPSPAWN_LOGV("RunExecutor in thread: %{public}d executorCount: %{public}u ",
        threadNode->index, mgr->executorCount);
    TaskExecuteNode *executor = PopTaskExecutor(mgr);
    uint32_t count = 0;
    while (executor != NULL && !threadNode->threadExit) {
        APPSPAWN_LOGV("RunExecutor task: %{public}u", executor->task->taskId);
        atomic_fetch_add(&executor->task->finishTaskCount, 1);
        executor->executor(executor->task->taskId, executor->context);
        count++;
        if (count >= maxCount) {
            break;
        }
        executor = PopTaskExecutor(mgr);
    }
    APPSPAWN_LOGV("RunExecutor executorCount: %{public}u end", mgr->executorCount);
}

static int TaskCompareTaskId(ListNode *node, void *data)
{
    TaskNode *task = ListEntry(node, TaskNode, node);
    return task->taskId - *(uint32_t *)data;
}

static TaskNode *GetTask(ThreadManager *mgr, ListNode *queue, uint32_t taskId)
{
    ListNode *node = NULL;
    pthread_mutex_lock(&mgr->mutex);
    node = OH_ListFind(queue, &taskId, TaskCompareTaskId);
    pthread_mutex_unlock(&mgr->mutex);
    if (node == NULL) {
        return NULL;
    }
    return ListEntry(node, TaskNode, node);
}

static void DeleteTask(TaskNode *task)
{
    APPSPAWN_LOGV("DeleteTask task: %{public}u ", task->taskId);

    if (!ListEmpty(task->node)) {
        return;
    }
    OH_ListRemoveAll(&task->executorList, NULL);
    pthread_cond_destroy(&task->cond);
    pthread_mutex_destroy(&task->mutex);
    free(task);
}

static TaskNode *PopTask(ThreadManager *mgr, ListNode *queue)
{
    TaskNode *task = NULL;
    pthread_mutex_lock(&mgr->mutex);
    ListNode *node = queue->next;
    if (node != queue) {
        OH_ListRemove(node);
        OH_ListInit(node);
        task = ListEntry(node, TaskNode, node);
    }
    pthread_mutex_unlock(&mgr->mutex);
    return task;
}

static void PushTask(ThreadManager *mgr, TaskNode *task, ListNode *queue)
{
    pthread_mutex_lock(&mgr->mutex);
    OH_ListAddTail(queue, &task->node);
    pthread_cond_broadcast(&mgr->cond);
    pthread_mutex_unlock(&mgr->mutex);
}

static void SafeRemoveTask(ThreadManager *mgr, TaskNode *task)
{
    pthread_mutex_lock(&mgr->mutex);
    OH_ListRemove(&task->node);
    OH_ListInit(&task->node);
    pthread_mutex_unlock(&mgr->mutex);

    ListNode *node = task->executorList.next;
    while (node != &task->executorList) {
        OH_ListRemove(node);
        OH_ListInit(node);
        TaskExecuteNode *executor = ListEntry(node, TaskExecuteNode, node);
        pthread_mutex_lock(&mgr->mutex);
        if (!ListEmpty(executor->executeNode)) {
            OH_ListRemove(&executor->executeNode);
            OH_ListInit(&executor->executeNode);
            mgr->executorCount--;
        }
        pthread_mutex_unlock(&mgr->mutex);
        free(executor);

        node = task->executorList.next;
    }
}

static void ExecuteTask(ThreadManager *mgr)
{
    TaskNode *task = PopTask(mgr, &mgr->waitingTaskQueue);
    if (task == NULL) {
        return;
    }

    APPSPAWN_LOGV("ExecuteTask task: %{public}u ", task->taskId);
    AddExecutor(mgr, task);
    PushTask(mgr, task, &mgr->executingTaskQueue);
    return;
}

static void CheckTaskComplete(ThreadManager *mgr)
{
    TaskNode *task = PopTask(mgr, &mgr->executingTaskQueue);
    if (task == NULL) {
        return;
    }
    if (task->totalTask <= atomic_load(&task->finishTaskCount)) {
        if (task->finishProcess != NULL) {
            task->finishProcess(task->taskId, task->context);
            DeleteTask(task);
            return;
        }
        pthread_mutex_lock(&task->mutex);
        pthread_cond_signal(&task->cond);
        pthread_mutex_unlock(&task->mutex);
        return;
    }
    PushTask(mgr, task, &mgr->executingTaskQueue);
    return;
}

static void TaskQueueDestroyProc(ListNode *node)
{
    OH_ListRemove(node);
    TaskNode *task = ListEntry(node, TaskNode, node);
    DeleteTask(task);
}

int CreateThreadMgr(uint32_t maxThreadCount, ThreadMgr *instance)
{
    if (g_threadManager != NULL) {
        *instance = (ThreadMgr)g_threadManager;
        return 0;
    }

    ThreadManager *mgr = (ThreadManager *)malloc(sizeof(ThreadManager) + maxThreadCount * sizeof(ThreadNode));
    APPSPAWN_CHECK(mgr != NULL, return -1, "Failed to create thread manager");

    mgr->executorCount = 0;
    mgr->currTaskId = 0;
    mgr->validThreadCount = 0;
    mgr->maxThreadCount = maxThreadCount;
    OH_ListInit(&mgr->taskList);
    OH_ListInit(&mgr->waitingTaskQueue);
    OH_ListInit(&mgr->executingTaskQueue);
    OH_ListInit(&mgr->executorQueue);
    pthread_mutex_init(&mgr->mutex, NULL);
    SetCondAttr(&mgr->cond);

    for (uint32_t index = 0; index < maxThreadCount + 1; index++) {
        mgr->threadNode[index].index = index;
        mgr->threadNode[index].threadId = INVALID_THREAD_ID;
        atomic_init(&mgr->threadNode[index].threadExit, 0);
    }
    g_threadManager = mgr;
    int ret = pthread_create(&mgr->threadNode[0].threadId, NULL, ManagerThreadProc, (void *)&mgr->threadNode[0]);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to create thread for manager");
        g_threadManager = NULL;
        free(mgr);
        return -1;
    }
    *instance = (ThreadMgr)mgr;
    APPSPAWN_LOGV("Create thread manager success maxThreadCount: %{public}u", maxThreadCount);
    return 0;
}

int DestroyThreadMgr(ThreadMgr instance)
{
    APPSPAWN_LOGV("DestroyThreadMgr");
    ThreadManager *mgr = (ThreadManager *)instance;
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid thread manager");

    for (uint32_t index = 0; index < mgr->maxThreadCount + 1; index++) {
        if (mgr->threadNode[index].threadId != INVALID_THREAD_ID) {
            atomic_store(&mgr->threadNode[index].threadExit, 1);
            APPSPAWN_LOGV("DestroyThreadMgr index %{public}d %{public}d", index, mgr->threadNode[index].threadExit);
        }
    }
    pthread_mutex_lock(&mgr->mutex);
    pthread_cond_broadcast(&mgr->cond);
    pthread_mutex_unlock(&mgr->mutex);
    for (uint32_t index = 0; index < mgr->maxThreadCount + 1; index++) {
        if (mgr->threadNode[index].threadId != INVALID_THREAD_ID) {
            pthread_join(mgr->threadNode[index].threadId, NULL);
            APPSPAWN_LOGV("DestroyThreadMgr index %{public}d end", index);
        }
    }

    pthread_mutex_lock(&mgr->mutex);
    OH_ListRemoveAll(&mgr->taskList, TaskQueueDestroyProc);
    OH_ListRemoveAll(&mgr->waitingTaskQueue, TaskQueueDestroyProc);
    OH_ListRemoveAll(&mgr->executingTaskQueue, TaskQueueDestroyProc);
    OH_ListRemoveAll(&mgr->executorQueue, TaskQueueDestroyProc);
    pthread_mutex_unlock(&mgr->mutex);

    pthread_cond_destroy(&mgr->cond);
    pthread_mutex_destroy(&mgr->mutex);
    return 0;
}

int ThreadMgrAddTask(ThreadMgr instance, ThreadTaskHandle *taskHandle)
{
    ThreadManager *mgr = (ThreadManager *)instance;
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid thread manager");
    TaskNode *task = (TaskNode *)malloc(sizeof(TaskNode));
    APPSPAWN_CHECK(task != NULL, return -1, "Failed to create thread task");

    task->context = NULL;
    task->finishProcess = NULL;
    task->totalTask = 0;
    atomic_init(&task->taskFlags, 0);
    atomic_init(&task->finishTaskCount, 0);
    OH_ListInit(&task->node);
    OH_ListInit(&task->executorList);
    pthread_mutex_init(&task->mutex, NULL);
    SetCondAttr(&task->cond);

    pthread_mutex_lock(&mgr->mutex);
    task->taskId = mgr->currTaskId++;
    OH_ListAddTail(&mgr->taskList, &task->node);
    pthread_mutex_unlock(&mgr->mutex);
    *taskHandle = task->taskId;
    APPSPAWN_LOGV("Create thread task success task id: %{public}u", task->taskId);
    return 0;
}

int ThreadMgrAddExecutor(ThreadMgr instance,
    ThreadTaskHandle taskHandle, TaskExecutor executor, const ThreadContext *context)
{
    ThreadManager *mgr = (ThreadManager *)instance;
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid thread manager");
    TaskNode *task = GetTask(mgr, &mgr->taskList, taskHandle);
    APPSPAWN_CHECK(task != NULL, return -1, "Invalid thread task %{public}u", taskHandle);

    TaskExecuteNode *node = (TaskExecuteNode *)malloc(sizeof(TaskExecuteNode));
    APPSPAWN_CHECK(node != NULL, return -1, "Failed to create thread executor for task %{public}u", taskHandle);
    node->task = task;
    OH_ListInit(&node->node);
    OH_ListInit(&node->executeNode);
    node->context = context;
    node->executor = executor;
    task->totalTask++;
    OH_ListAddTail(&task->executorList, &node->node);
    return 0;
}

int ThreadMgrCancelTask(ThreadMgr instance, ThreadTaskHandle taskHandle)
{
    ThreadManager *mgr = (ThreadManager *)instance;
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid thread manager");
    TaskNode *task = GetTask(mgr, &mgr->taskList, taskHandle);
    if (task != NULL) {
        SafeRemoveTask(mgr, task);
        DeleteTask(task);
        return 0;
    }
    task = GetTask(mgr, &mgr->waitingTaskQueue, taskHandle);
    if (task != NULL) {
        SafeRemoveTask(mgr, task);
        DeleteTask(task);
        return 0;
    }
    task = GetTask(mgr, &mgr->executingTaskQueue, taskHandle);
    if (task != NULL) {
        SafeRemoveTask(mgr, task);
        DeleteTask(task);
        return 0;
    }
    return 0;
}

int TaskSyncExecute(ThreadMgr instance, ThreadTaskHandle taskHandle)
{
    ThreadManager *mgr = (ThreadManager *)instance;
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid thread manager");
    TaskNode *task = GetTask(mgr, &mgr->taskList, taskHandle);
    APPSPAWN_CHECK(task != NULL, return -1, "Invalid thread task %{public}u", taskHandle);

    pthread_mutex_lock(&task->mutex);
    OH_ListRemove(&task->node);
    OH_ListInit(&task->node);
    OH_ListAddTail(&mgr->waitingTaskQueue, &task->node);
    pthread_cond_broadcast(&mgr->cond);
    pthread_mutex_unlock(&task->mutex);
    APPSPAWN_LOGV("TaskSyncExecute task: %{public}u", task->taskId);
    struct timespec abstime;
    int ret = 0;
    do {
        ConvertToTimespec(60 * 1000, &abstime);  // wait 60 * 1000 60s
        pthread_mutex_lock(&task->mutex);
        ret = pthread_cond_timedwait(&task->cond, &task->mutex, &abstime);
        pthread_mutex_unlock(&task->mutex);
        APPSPAWN_LOGV("TaskSyncExecute success task id: %{public}u ret: %{public}d", task->taskId, ret);
    } while (ret == ETIMEDOUT);

    DeleteTask(task);
    return ret;
}

int TaskExecute(ThreadMgr instance,
    ThreadTaskHandle taskHandle, TaskFinishProcessor process, const ThreadContext *context)
{
    ThreadManager *mgr = (ThreadManager *)instance;
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid thread manager");
    TaskNode *task = GetTask(mgr, &mgr->taskList, taskHandle);
    APPSPAWN_CHECK(task != NULL, return -1, "Invalid thread task %{public}u", taskHandle);

    task->finishProcess = process;
    task->context = context;
    pthread_mutex_lock(&mgr->mutex);
    OH_ListRemove(&task->node);
    OH_ListInit(&task->node);
    OH_ListAddTail(&mgr->waitingTaskQueue, &task->node);
    pthread_cond_broadcast(&mgr->cond);
    pthread_mutex_unlock(&mgr->mutex);
    APPSPAWN_LOGV("TaskExecute task: %{public}u", task->taskId);
    return 0;
}

static void CheckAndCreateNewThread(ThreadManager *mgr)
{
    if (mgr->maxThreadCount <= mgr->validThreadCount) {
        return;
    }
    if (mgr->executorCount <= mgr->validThreadCount) {
        return;
    }
    APPSPAWN_LOGV("CheckAndCreateNewThread maxThreadCount: %{public}u validThreadCount: %{public}u %{public}u",
        mgr->maxThreadCount, mgr->validThreadCount, mgr->executorCount);

    uint32_t totalThread = mgr->maxThreadCount;
    if (mgr->executorCount <= mgr->maxThreadCount) {
        totalThread = mgr->executorCount;
    }

    for (uint32_t index = 0; index < mgr->maxThreadCount + 1; index++) {
        if (mgr->threadNode[index].threadId != INVALID_THREAD_ID) {
            continue;
        }
        int ret = pthread_create(&mgr->threadNode[index].threadId,
            NULL, ThreadExecute, (void *)&(mgr->threadNode[index]));
        APPSPAWN_CHECK(ret == 0, return, "Failed to create thread for %{public}u", index);
        APPSPAWN_LOGV("Create thread success index: %{public}u", mgr->threadNode[index].index);
        mgr->validThreadCount++;
        if (mgr->validThreadCount >= totalThread) {
            return;
        }
    }
    return;
}

static void *ManagerThreadProc(void *args)
{
    ThreadManager *mgr = g_threadManager;
    ThreadNode *threadNode = (ThreadNode *)args;
    struct timespec abstime;
    int ret = 0;
    while (!threadNode->threadExit) {
        pthread_mutex_lock(&mgr->mutex);
        do {
            uint32_t timeout = 60 * 1000; // 60 * 1000 60s
            if (!ListEmpty(mgr->waitingTaskQueue)) {
                break;
            }
            if (!ListEmpty(mgr->executingTaskQueue)) {
                timeout = 500; // 500ms
            }
            ConvertToTimespec(timeout, &abstime);
            ret = pthread_cond_timedwait(&mgr->cond, &mgr->mutex, &abstime);
            if (!ListEmpty(mgr->executingTaskQueue) || ret == ETIMEDOUT) {
                break;
            }
            if (threadNode->threadExit) {
                break;
            }
        } while (1);
        pthread_mutex_unlock(&mgr->mutex);

        ExecuteTask(mgr);
        CheckAndCreateNewThread(mgr);

        if (mgr->validThreadCount == 0) {
            RunExecutor(mgr, threadNode, 5); // 5 max thread
        }
        CheckTaskComplete(mgr);
    }
    return 0;
}

static void *ThreadExecute(void *args)
{
    ThreadManager *mgr = g_threadManager;
    ThreadNode *threadNode = (ThreadNode *)args;
    struct timespec abstime;
    int ret = 0;
    while (!threadNode->threadExit) {
        pthread_mutex_lock(&mgr->mutex);
        while (ListEmpty(mgr->executorQueue) && !threadNode->threadExit) {
            ConvertToTimespec(60 * 1000, &abstime); // 60 * 1000 60s
            ret = pthread_cond_timedwait(&mgr->cond, &mgr->mutex, &abstime);
        }
        pthread_mutex_unlock(&mgr->mutex);
        APPSPAWN_LOGV("bbbb threadNode->threadExit %{public}d", threadNode->threadExit);
        RunExecutor(mgr, threadNode, 1);
    }
    return NULL;
}
