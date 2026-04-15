# Checkpoint 测试用例汇总表

## 1. app_spawn_checkpoint_req_test.cpp (12个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| Process_Checkpoint_Req_Msg_001 | 测试MSG_SPAWN_IMAGE_PROCESS消息处理成功路径 | 创建AppSpawnMgr，构造完整TLV消息，Mock ioctl返回成功 | 消息处理成功，返回resultPid=54321 |
| Process_Checkpoint_Req_Msg_002 | 测试MSG_SPAWN_WORKER_PROCESS消息处理成功路径 | 创建AppSpawnMgr，构造WORKER_PROCESS类型消息 | 消息处理成功，返回resultPid=54322 |
| Process_Checkpoint_Req_Msg_003 | 测试空进程名时CheckAppSpawnMsg失败分支 | 创建消息但进程名为空 | 函数提前返回并发送错误响应 |
| Process_Checkpoint_Req_Msg_004 | 测试无效消息类型时CheckAppSpawnMsg失败分支 | 创建消息但msgType=MAX_TYPE_INVALID | 函数提前返回并发送错误响应 |
| Process_Checkpoint_Req_Msg_005 | 测试缺少checkpoint info时hook执行失败 | 创建消息但不设置checkpoint info，Mock ioctl检查inputPid | hook执行失败 |
| Process_Checkpoint_Req_Msg_006 | 测试ioctl返回EIO错误时处理 | 构造完整消息，Mock ioctl返回-1并设置errno=EIO | hook执行失败，正确处理错误 |
| Process_Checkpoint_Req_Msg_007 | 测试空连接指针边界条件 | 构造有效消息，传入空连接指针 | 函数优雅处理，不崩溃 |
| Process_Checkpoint_Req_Msg_008 | 测试空消息指针边界条件 | 创建连接对象，传入空消息指针 | 函数优雅处理，不崩溃 |
| Process_Checkpoint_Req_Msg_009 | 测试响应中checkPointId验证 | 构造指定checkPointId=12345678的消息 | 响应正确包含checkPointId |
| Process_Checkpoint_Req_Msg_010 | 测试ioctl返回ENOMEM错误 | 构造完整消息，Mock ioctl返回ENOMEM | hook执行失败，正确处理内存不足错误 |
| Process_Checkpoint_Req_Msg_011 | 测试ioctl返回EPERM错误 | 构造完整消息，Mock ioctl返回EPERM | hook执行失败，正确处理权限错误 |
| Process_Checkpoint_Req_Msg_Stress_001 | 压力测试：连续处理10个checkpoint请求 | 创建AppSpawnMgr，循环创建不同进程名的请求 | 所有请求均正确处理，无资源泄漏 |

---

## 2. app_spawn_img_info_client_test.cpp (22个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| App_ImgInfoClient_SendMsg_001 | 测试MSG_SPAWN_IMAGE_PROCESS类型消息发送 | 初始化客户端，创建消息并设置checkpoint info | AppSpawnClientSendMsg调用成功 |
| App_ImgInfoClient_SendMsg_002 | 测试MSG_SPAWN_WORKER_PROCESS类型消息发送 | 初始化客户端，创建WORKER消息 | AppSpawnClientSendMsg调用成功 |
| App_ImgInfoClient_SendMsg_003 | 测试空客户端句柄发送消息 | 创建消息但客户端句柄为nullptr | 返回非0错误码 |
| App_ImgInfoClient_SendMsg_004 | 测试空请求句柄发送消息 | 初始化客户端但reqHandle为nullptr | 返回非0错误码 |
| App_ImgInfoClient_SendMsg_005 | 测试INVALID_REQ_HANDLE发送消息 | 初始化客户端但reqHandle=INVALID_REQ_HANDLE | 返回非0错误码 |
| App_ImgInfoClient_SetImgInfo_001 | 测试AppSpawnReqMsgSetCheckpointInfo基本功能 | 创建MSG_SPAWN_IMAGE_PROCESS消息 | 设置成功，返回0 |
| App_ImgInfoClient_SetImgInfo_002 | 测试imgPid为0时设置失败 | 创建消息，传入imgPid=0 | 返回非0错误码 |
| App_ImgInfoClient_SetImgInfo_003 | 测试checkPointId为0时设置成功 | 创建消息，传入checkPointId=0 | 设置成功，返回0 |
| App_ImgInfoClient_SetImgInfo_004 | 测试空句柄设置checkpoint info | 传入nullptr作为reqHandle | 返回非0错误码 |
| App_ImgInfoClient_SetImgInfo_005 | 测试最大值参数设置 | 传入maxCheckPointId=0xFFFFFFFFFFFFFFFF | 设置成功，返回0 |
| App_ImgInfoClient_MsgWithImgInfo_001 | 测试IMAGE_PROCESS消息完整创建流程 | 初始化客户端，设置bundle info和checkpoint info及flags | 所有设置成功 |
| App_ImgInfoClient_MsgWithImgInfo_002 | 测试WORKER_PROCESS消息完整创建流程 | 初始化客户端，设置bundle info和checkpoint info | 所有设置成功 |
| App_ImgInfoClient_DestroyTest_001 | 测试客户端销毁后资源清理 | 创建消息后直接销毁客户端 | 无崩溃，资源正确清理 |
| App_ImgInfoClient_MultipleMsgTest_001 | 测试同一客户端创建多个消息 | 初始化客户端，循环创建5个不同消息 | 所有消息创建和设置成功 |
| App_ImgInfoClient_MessageTypeCombinations_001 | 测试不同消息类型组合 | 分别创建IMAGE_PROCESS和WORKER_PROCESS消息 | 两种类型均正确处理 |
| App_ImgInfoClient_StringProcessName_001 | 测试长进程名处理 | 使用超长进程名创建消息 | 创建成功 |
| App_ImgInfoClient_StringProcessName_002 | 测试特殊字符进程名处理 | 使用包含数字和点的进程名 | 创建成功 |
| App_ImgInfoClient_BundleInfo_001 | 测试SetBundleInfo与checkpoint info组合 | 设置bundle index=100和checkpoint info | 两者均设置成功 |
| App_ImgInfoClient_AppFlags_001 | 测试SetAppFlag与checkpoint info组合 | 设置COLD_BOOT和SPAWN_IMAGE_PROCESS标志 | 标志设置成功 |

---

## 3. app_spawn_img_info_test.cpp (26个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| App_ImgInfo_GetSpawningFd_001 | 测试空content参数 | 传入nullptr作为content | 返回nullptr |
| App_ImgInfo_GetSpawningFd_002 | 测试未添加fd时获取 | 创建AppSpawnMgr但未添加fd | 返回nullptr |
| App_ImgInfo_GetSpawningFd_003 | 测试添加fd后获取 | 添加fd=100后获取 | 返回正确的AppSpawnFds，type和count正确 |
| App_ImgInfo_AddSpawningFds_001 | 测试空content参数 | 传入nullptr作为content | 返回非0错误码 |
| App_ImgInfo_AddSpawningFds_002 | 测试无效type参数 | type=TYPE_INVALID | 返回非0错误码 |
| App_ImgInfo_AddSpawningFds_003 | 测试空fds参数 | fds=nullptr | 返回非0错误码 |
| App_ImgInfo_AddSpawningFds_004 | 测试有效参数添加多个fd | 添加2个fd：100, 200 | 添加成功，count=2 |
| App_ImgInfo_AddCheckPointInfo_001 | 测试空content参数 | content=nullptr | 返回nullptr |
| App_ImgInfo_AddCheckPointInfo_002 | 测试checkpointId=0 | checkpointId=0 | 返回nullptr |
| App_ImgInfo_AddCheckPointInfo_003 | 测试空进程名 | processName=nullptr | 返回nullptr |
| App_ImgInfo_AddCheckPointInfo_004 | 测试有效参数添加 | checkpointId=1001, name="com.test.app" | 添加成功，字段值正确 |
| App_ImgInfo_GetByName_001 | 测试空content参数 | content=nullptr | 返回nullptr |
| App_ImgInfo_GetByName_002 | 测试空name参数 | name=nullptr | 返回nullptr |
| App_ImgInfo_GetByName_003 | 测试查找不存在的进程名 | name="non_existent" | 返回nullptr |
| App_ImgInfo_GetByName_004 | 测试查找已添加的进程 | 先添加再查找 | 返回正确的checkpoint info |
| App_ImgInfo_Traversal_001 | 测试空content参数遍历 | content=nullptr | 遍历count=0 |
| App_ImgInfo_Traversal_002 | 测试空遍历函数 | traversal=nullptr | 遍历count=0 |
| App_ImgInfo_Traversal_003 | 测试遍历多个checkpoint进程 | 添加3个进程后遍历 | count=3 |
| App_ImgInfo_DestroyImgQue_001 | 测试空content参数销毁 | content=nullptr | 返回0 |
| App_ImgInfo_DestroyImgQue_002 | 测试错误mode销毁 | mode=MODE_FOR_NWEB_SPAWN | 返回0 |
| App_ImgInfo_DestroyImgQue_003 | 测试无fd时销毁 | 添加checkpoint info但无fd | 返回0 |
| App_ImgInfo_DestroyImgQue_004 | 测试有效参数销毁 | 添加checkpoint info和有效fd，Mock ioctl | 返回0 |
| App_ImgInfo_DestroyImgQue_005 | 测试无效fd销毁 | 添加checkpoint info和fd=-1 | 返回0 |
| App_ImgInfo_Lifecycle_001 | 测试完整生命周期 | 添加、查找、销毁完整流程 | 各步骤均成功 |
| App_ImgInfo_MultiProcess_001 | 测试多进程场景 | 添加10个不同进程 | 所有进程均可查找 |
| App_ImgInfo_Stress_001 | 压力测试50个进程 | 添加50个进程后遍历 | count=50 |

---

## 4. app_spawn_img_loader_test.cpp (26个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| App_ImgLoader_GetSpawningFd_001 | 测试空content参数 | content=nullptr | 返回nullptr |
| App_ImgLoader_GetSpawningFd_002 | 测试未添加fd时获取 | 创建AppSpawnMgr但未添加fd | 返回nullptr |
| App_ImgLoader_GetSpawningFd_003 | 测试添加fd后获取 | 添加fd=100后获取 | 返回正确结果，fds[0]=100 |
| App_ImgLoader_AddSpawningFds_001 | 测试无效type参数 | type=TYPE_INVALID | 返回APPSPAWN_ARG_INVALID |
| App_ImgLoader_AddSpawningFds_002 | 测试count=0 | count=0 | 返回APPSPAWN_ARG_INVALID |
| App_ImgLoader_AddSpawningFds_003 | 测试空fds参数 | fds=nullptr | 返回APPSPAWN_ARG_INVALID |
| App_ImgLoader_AddSpawningFds_004 | 测试添加多个fd | 添加3个fd：100,200,300 | 添加成功，count=3 |
| App_ImgLoader_AddCheckPointInfo_001 | 测试checkpointId=0 | checkpointId=0 | 返回nullptr |
| App_ImgLoader_AddCheckPointInfo_002 | 测试空进程名 | processName="" | 返回nullptr |
| App_ImgLoader_AddCheckPointInfo_003 | 测试有效参数添加 | checkpointId=1234, name="test_process" | 添加成功且可查找到 |
| App_ImgLoader_GetByName_001 | 测试空name参数 | name=nullptr | 返回nullptr |
| App_ImgLoader_GetByName_002 | 测试查找不存在的进程名 | name="non_existent" | 返回nullptr |
| App_ImgLoader_GetByName_003 | 测试查找已添加的进程 | 添加2个进程后分别查找 | 返回正确的checkpoint info |
| App_ImgLoader_Traversal_001 | 测试空content参数遍历 | content=nullptr | count=0 |
| App_ImgLoader_Traversal_002 | 测试空遍历函数 | traversal=nullptr | count=0 |
| App_ImgLoader_Traversal_003 | 测试遍历多个checkpoint进程 | 添加3个进程后遍历 | count=3 |
| App_ImgLoader_DestroyImgQue_001 | 测试空content参数销毁 | content=nullptr | 返回0 |
| App_ImgLoader_DestroyImgQue_002 | 测试错误mode销毁 | mode=MODE_FOR_NWEB_SPAWN | 返回0 |
| App_ImgLoader_DestroyImgQue_003 | 测试无fd时销毁 | 添加checkpoint info但无fd | 返回0 |
| App_ImgLoader_DestroyImgQue_004 | 测试有效参数销毁 | 添加checkpoint info和有效fd | 返回0 |
| App_ImgLoader_DestroyImgQue_005 | 测试无效fd销毁 | 添加checkpoint info和fd=-1 | 返回0 |
| App_ImgLoader_MultiProcess_001 | 测试多进程场景 | 添加10个不同进程 | 所有进程均可查找 |
| App_ImgLoader_MultiFdType_001 | 测试多种fd类型 | 添加TYPE_FOR_DEC和TYPE_FOR_FORK_ALL | 两种类型均可正确获取 |
| App_ImgLoader_LargeFdCount_001 | 测试大量fd | 添加10个fd | count=10，值正确 |
| App_ImgLoader_LongProcessName_001 | 测试长进程名 | 使用超长进程名 | 添加成功且可查找 |
| App_ImgLoader_Lifecycle_001 | 测试完整生命周期 | 添加、查找、销毁完整流程 | 各步骤均成功 |
| App_ImgLoader_Stress_001 | 压力测试50个进程 | 添加50个进程后遍历 | count=50 |
| App_ImgLoader_LargeCheckpointId_001 | 测试最大checkpointId | checkpointId=0xFFFFFFFFFFFFFFFF | 添加成功且值正确 |
| App_ImgLoader_SpecialCharProcessName_001 | 测试特殊字符进程名 | 使用数字和下划线组合 | 添加成功且可查找 |
| App_ImgLoader_SortedOrder_001 | 测试队列排序顺序 | 非顺序添加3个进程 | 遍历结果按checkpointId排序 |

---

## 5. app_spawn_msg_type_test.cpp (16个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| App_Spawn_Msg_Type_Image_Process_Create_001 | 测试MSG_SPAWN_IMAGE_PROCESS消息创建 | processName="com.test.imgapp" | 创建成功，reqHandle非空 |
| App_Spawn_Msg_Type_Worker_Process_Create_001 | 测试MSG_SPAWN_WORKER_PROCESS消息创建 | processName="com.test.workerapp" | 创建成功，reqHandle非空 |
| App_Spawn_Msg_Type_Invalid_Name_001 | 测试空进程名创建消息 | processName="" | 创建失败，返回非0 |
| App_Spawn_Msg_Set_Img_Info_Valid_001 | 测试有效参数设置checkpoint info | 创建IMAGE_PROCESS消息，设置imgPid和checkPointId | 设置成功，返回0 |
| App_Spawn_Msg_Set_Img_Info_Null_Handle_001 | 测试空句柄设置checkpoint info | reqHandle=nullptr | 返回非0错误码 |
| App_Spawn_Msg_Set_Img_Info_Zero_Pid_001 | 测试imgPid=0时设置 | imgPid=0 | 返回非0错误码 |
| App_Spawn_Msg_Set_Img_Info_Zero_Checkpoint_001 | 测试checkPointId=0时设置 | checkPointId=0 | 设置成功，返回0 |
| App_Spawn_Msg_Flags_Spawn_Img_Info_001 | 测试设置SPAWN_IMAGE_PROCESS标志 | 创建消息后设置APP_FLAGS_SPAWN_IMAGE_PROCESS | 设置成功，返回0 |
| App_Spawn_Msg_Worker_With_Img_Info_001 | 测试WORKER_PROCESS配合checkpoint info | 创建WORKER消息并设置checkpoint info | 设置成功 |
| App_Spawn_Msg_Multi_Set_Img_Info_001 | 测试同一消息多次设置checkpoint info | 连续设置两次不同值 | 两次均成功，值被更新 |
| App_Spawn_Msg_Worker_Bundle_Info_001 | 测试WORKER消息设置bundle info | 设置bundle info和checkpoint info | 两者均成功 |
| App_Spawn_Msg_Special_Chars_In_Name_001 | 测试特殊字符进程名 | 使用数字、点、下划线组合的进程名 | 所有进程名创建成功 |
| App_Spawn_Msg_Img_Related_Flags_001 | 测试镜像相关标志设置 | 设置APP_FLAGS_SPAWN_IMAGE_PROCESS | 设置成功 |
| App_Spawn_Msg_Worker_Without_Img_Info_001 | 测试WORKER消息不设置checkpoint info | 创建消息但不调用SetCheckpointInfo | 创建成功 |
| App_Spawn_Msg_Image_Missing_Info_001 | 测试IMAGE_PROCESS消息缺少checkpoint info | 创建消息但不设置checkpoint info | 创建成功 |
| App_Spawn_Msg_Worker_With_Cold_Boot_001 | 测试WORKER与冷启动标志组合 | 设置COLD_BOOT标志和checkpoint info | 两者均成功 |
| App_Spawn_Msg_Type_Routing_Diff_001 | 测试不同消息类型路由差异 | 分别创建IMAGE、WORKER、SPAWN三种类型 | 三种类型句柄均非空 |

---

## 6. app_spawn_checkpoint_process_test.cpp (39个用例)

### DoCheckpointProcess 函数测试 (11个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| DoCheckpointProcess_001 | 测试content为空指针 | property有效，content=nullptr | 返回APPSPAWN_ARG_INVALID |
| DoCheckpointProcess_002 | 测试property为空指针 | content有效，property=nullptr | 返回APPSPAWN_ARG_INVALID |
| DoCheckpointProcess_003 | 测试message为空指针 | 创建AppSpawningCtx但message=nullptr | 返回APPSPAWN_ARG_INVALID |
| DoCheckpointProcess_004 | 测试缺少checkpoint TLV信息 | 创建不含TLV_CHECK_POINT_INFO的消息 | 返回APPSPAWN_ARG_INVALID |
| DoCheckpointProcess_005 | 测试ioctl返回EIO错误 | 构造完整消息，Mock ioctl返回-1并设置errno=EIO | 返回EIO |
| DoCheckpointProcess_006 | 测试image process成功路径(needForkDenied=false) | 构造IMAGE_PROCESS消息，Mock ioctl返回resultPid=54321 | 返回0，pid=54321，checkPointId正确 |
| DoCheckpointProcess_007 | 测试worker process成功路径(needForkDenied=true) | 构造WORKER_PROCESS消息，Mock ioctl返回resultPid=54322 | 返回0，pid=54322，checkPointId正确 |
| DoCheckpointProcess_008 | 测试已有checkpoint info且checkPointId不同时更新 | 预添加checkPointId=9999的节点，传入checkPointId=2000 | 返回0，已有节点checkPointId更新为2000 |
| DoCheckpointProcess_009 | 测试无已有checkpoint info时新增节点 | 不预添加节点，传入checkPointId=3000 | 返回0，新节点被添加且checkPointId=3000 |
| DoCheckpointProcess_010 | 测试已有checkpoint info且checkPointId相同时不更新 | 预添加checkPointId=4000的节点，传入相同checkPointId | 返回0，节点checkPointId保持4000不变 |
| DoCheckpointProcess_011 | 测试ioctl返回ENOMEM错误 | 构造完整消息，Mock ioctl返回ENOMEM | 返回ENOMEM |

### CreateImageProcess 函数测试 (3个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| CreateImageProcess_001 | 测试CreateImageProcess成功路径 | 构造完整消息，Mock ioctl返回resultPid=60001 | 返回0，pid=60001，checkPointId正确 |
| CreateImageProcess_002 | 测试CreateImageProcess content为空 | content=nullptr | 返回APPSPAWN_ARG_INVALID |
| CreateImageProcess_003 | 测试CreateImageProcess ioctl失败 | Mock ioctl返回EINVAL | 返回EINVAL |

### CreateWorkerProcess 函数测试 (3个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| CreateWorkerProcess_001 | 测试CreateWorkerProcess成功路径 | 构造完整消息，Mock ioctl返回resultPid=70001 | 返回0，pid=70001，checkPointId正确 |
| CreateWorkerProcess_002 | 测试CreateWorkerProcess property为空 | property=nullptr | 返回APPSPAWN_ARG_INVALID |
| CreateWorkerProcess_003 | 测试CreateWorkerProcess ioctl失败 | Mock ioctl返回EPERM | 返回EPERM |

### SpawnPrepareCheckpointFd 函数测试 (7个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| SpawnPrepareCheckpointFd_001 | 测试content为空指针 | content=nullptr | 返回APPSPAWN_ARG_INVALID |
| SpawnPrepareCheckpointFd_002 | 测试property为空指针 | property=nullptr | 返回APPSPAWN_ARG_INVALID |
| SpawnPrepareCheckpointFd_003 | 测试非APP_SPAWN模式 | mode=MODE_FOR_NWEB_SPAWN | 返回APPSPAWN_ARG_INVALID |
| SpawnPrepareCheckpointFd_004 | 测试无checkpoint info(非checkpoint请求) | 创建不含checkpoint TLV的消息 | 返回0，不处理 |
| SpawnPrepareCheckpointFd_005 | 测试fd已缓存场景 | 预添加TYPE_FOR_FORK_ALL的fd=100 | 返回0，fd保持不变 |
| SpawnPrepareCheckpointFd_006 | 测试open设备文件失败 | Mock open返回-1并设置errno=ENOENT | 返回APPSPAWN_SYSTEM_ERROR |
| SpawnPrepareCheckpointFd_007 | 测试open设备文件成功路径 | Mock open返回fake fd=42 | 返回0，fd=42被正确缓存 |

### CreateImageProcessHook 函数测试 (6个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| CreateImageProcessHook_001 | 测试content为空指针 | content=nullptr | 返回APPSPAWN_ARG_INVALID |
| CreateImageProcessHook_002 | 测试property为空指针 | property=nullptr | 返回APPSPAWN_ARG_INVALID |
| CreateImageProcessHook_003 | 测试非APP_SPAWN模式 | mode=MODE_FOR_NWEB_SPAWN | 返回APPSPAWN_ARG_INVALID |
| CreateImageProcessHook_004 | 测试APP_FLAGS_SPAWN_IMAGE_PROCESS未设置(非镜像进程) | 创建不含SPAWN_IMAGE_PROCESS标志的消息 | 返回0，跳过处理 |
| CreateImageProcessHook_005 | 测试checkpoint fd未准备 | 添加checkpoint消息但不缓存fd | 返回APPSPAWN_SYSTEM_ERROR |
| CreateImageProcessHook_006 | 测试镜像进程Hook成功路径 | 构造完整消息，缓存fd=42，Mock ioctl返回resultPid=80001 | 返回0，pid=80001 |

### CreateWorkerProcessHook 函数测试 (7个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| CreateWorkerProcessHook_001 | 测试content为空指针 | content=nullptr | 返回APPSPAWN_ARG_INVALID |
| CreateWorkerProcessHook_002 | 测试property为空指针 | property=nullptr | 返回APPSPAWN_ARG_INVALID |
| CreateWorkerProcessHook_003 | 测试非APP_SPAWN模式 | mode=MODE_FOR_NWEB_SPAWN | 返回APPSPAWN_ARG_INVALID |
| CreateWorkerProcessHook_004 | 测试APP_FLAGS_SPAWN_IMAGE_PROCESS已设置(镜像进程跳过) | 创建含SPAWN_IMAGE_PROCESS标志的checkpoint消息 | 返回0，跳过处理 |
| CreateWorkerProcessHook_005 | 测试无checkpoint info(非checkpoint请求) | 创建不含checkpoint TLV的消息 | 返回0，跳过处理 |
| CreateWorkerProcessHook_006 | 测试checkpoint fd未准备 | 添加checkpoint消息但不缓存fd | 返回APPSPAWN_SYSTEM_ERROR |
| CreateWorkerProcessHook_007 | 测试工作进程Hook成功路径 | 构造WORKER消息，缓存fd=42，Mock ioctl返回resultPid=90001 | 返回0，pid=90001 |

### 端到端集成测试 (2个用例)

| 测试用例名称 | 测试目的 | 预置条件 | 预期结果 |
|-------------|---------|---------|---------|
| CheckpointProcess_E2E_001 | 测试完整链路：prepare fd → create image process hook | 创建AppSpawnMgr，Mock open返回fd=99，Mock ioctl返回resultPid=111111 | prepareRet=0，hookRet=0，pid=111111，checkPointInfo被正确添加 |
| CheckpointProcess_E2E_002 | 测试完整链路：prepare fd → create worker process hook | 创建AppSpawnMgr，Mock open返回fd=88，Mock ioctl返回resultPid=222222 | prepareRet=0，hookRet=0，pid=222222，checkPointId正确 |

---

## 统计汇总

| 测试文件 | 用例数量 |
|---------|---------|
| app_spawn_checkpoint_req_test.cpp | 12 |
| app_spawn_img_info_client_test.cpp | 22 |
| app_spawn_img_info_test.cpp | 26 |
| app_spawn_img_loader_test.cpp | 26 |
| app_spawn_msg_type_test.cpp | 16 |
| app_spawn_checkpoint_process_test.cpp | 39 |
| **总计** | **141** |

---

## 测试覆盖范围

### 功能覆盖
- **消息类型测试**: MSG_SPAWN_IMAGE_PROCESS, MSG_SPAWN_WORKER_PROCESS
- **API接口测试**: AppSpawnClientInit, AppSpawnClientSendMsg, AppSpawnClientDestroy, AppSpawnReqMsgCreate, AppSpawnReqMsgSetCheckpointInfo
- **内部函数测试**: GetSpawningFd, AddSpawningFds, AddSpawningCheckPointInfo, GetSpawningImgInfoByName, TraversalImgProcess, SpawnDestoryImgQue
- **消息处理测试**: ProcessCheckpointReqMsg
- **进程创建测试**: DoCheckpointProcess, CreateImageProcess, CreateWorkerProcess
- **Hook函数测试**: SpawnPrepareCheckpointFd, CreateImageProcessHook, CreateWorkerProcessHook
- **端到端测试**: prepare fd → image process hook链路, prepare fd → worker process hook链路

### 边界条件覆盖
- 空指针参数
- 无效参数值（type, count, fds等）
- 零值参数（checkpointId=0, imgPid=0）
- 最大值参数
- 特殊字符进程名
- 长进程名

### 错误处理覆盖
- ioctl返回各种错误码（EIO, ENOMEM, EPERM, EINVAL）
- open设备文件失败（ENOENT）
- 参数验证失败
- 资源分配失败
- checkpoint fd未准备场景
- checkpoint info缺失场景

### 性能测试
- 多进程场景（10-50个进程）
- 大量fd测试
- 压力测试
