#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import optparse
import os
import sys
import json
import stat

sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
    os.pardir, os.pardir, os.pardir, "build"))
from scripts.util import build_utils  # noqa: E402

#default json

APP_SANDBOX_DEFAULT = '''
{
    "common" : [{
        "top-sandbox-switch": "ON",
        "app-base" : [{
            "sandbox-root" : "/mnt/sandbox/<currentUserId>/<PackageName>",
            "sandbox-ns-flags" : [],
            "mount-paths" : [],
            "symbol-links": [],
            "flags-point" : []
        }],
        "app-resources" : [{
            "sandbox-root" : "/mnt/sandbox/<currentUserId>/<PackageName>",
            "mount-paths" : [],
            "flags-point" : [],
            "symbol-links" : []
        }]
    }],
    "individual" : [{}],
    "permission" :[{}]
}
'''
#only string in list

def _merge_list(origin, new):
    if origin is None or new is None:
        return
    for data1 in new:
        if data1 not in origin:
            origin.append(data1)

def _is_same_data(data1, data2, keys):
    for key in keys:
        if data1.get(key) != data2.get(key):
            return False
    return True

#for object in list

def _handle_same_array(data1, data2):
    for field in ["sandbox-root", "sandbox-path", "check-action-status", "fs-type", "link-name"]:
        if data1.get(field) is not None:
            data2[field] = data1[field]

    for field in ["sandbox-flags"]: # by list merger
        item = data1.get(field)
        if item is not None and len(item) > 0:
            _merge_list(data2[field], item)

def _merge_scope_array(origin, new, keys):
    for data1 in new:
        found = False
        for data2 in origin:
            if _is_same_data(data1, data2, keys):
                found = True
                _handle_same_array(data1, data2)
                break
        if not found:
            origin.append(data1)

def _handle_same_data(data1, data2, field_infos):
    for field in ["sandbox-root"]:
        if data1.get(field) is not None:
            data2[field] = data1[field]

    # for array
    for name, keys in field_infos.items():
        item = data1.get(name)
        if item is not None and len(item) > 0:
            _merge_scope_array(data2[field], item, keys)

def _merge_scope_flags_point(origin, new):
    field_infos = {
        "mount-paths": ["src-path"]
    }
    for data1 in new:
        found = False
        for data2 in origin:
            if _is_same_data(data1, data2, ["flags"]):
                found = True
                _handle_same_data(data1, data2, field_infos)
                break

        if not found:
            origin.append(data1)

def _merge_scope_app(origin, new):
    field_infos = {
        "mount-paths": ["src-path"],
        "symbol-links": ["target-name"]
    }
    # normal filed
    for k in ["sandbox-root", "sandbox-switch", "gids"]:
        if new[0].get(k) is not None:
            origin[0][k] = new[0].get(k)

    # for flags-point
    flags_points = new[0].get("flags-point")
    if flags_points:
        _merge_scope_flags_point(origin[0]["flags-point"], flags_points)

    # for array
    for name, keys in field_infos.items():
        item = new[0].get(name)
        if item is not None and len(item) > 0:
            _merge_scope_array(origin[0].get(name), item, keys)

def _merge_scope_individual(origin, new):
    for k, v in new.items():
        if k not in origin:
            origin[k] = v
        else:
            _merge_scope_app(origin[k], v)


def _merge_scope_permission(origin, new):
    for k, v in new.items():
        if k not in origin:
            origin[k] = v
        else:
            _merge_scope_app(origin[k], v)

def _merge_scope_common(origin, new):
    # 处理 top-sandbox-switch
    for name in ["top-sandbox-switch"]:
        if new.get(name) :
            origin[name] = new.get(name)

    #处理 app-base
    app = new.get("app-base")
    if  app is not None and len(app) > 0:
        _merge_scope_app(origin.get("app-base"), app)
        pass

    #处理 app-resources
    app = new.get("app-resources")
    if  app is not None and len(app) > 0:
        _merge_scope_app(origin.get("app-resources"), app)
        pass

def parse_args(args):
    args = build_utils.expand_file_args(args)
    parser = optparse.OptionParser()
    build_utils.add_depfile_option(parser)
    parser.add_option('--output', help='fixed sandbox configure file')
    parser.add_option('--source-file', help='source para file')
    parser.add_option('--patterns', action="append",
        type="string", dest="patterns", help='replace string patterns like libpath:lib64')
    parser.add_option('--extra_sandbox_cfg', action="append",
        type="string", dest="extra_sandbox_cfgs", help='extra sandbox')

    options, _ = parser.parse_args(args)
    return options

def __substitude_contents(options, source_file):
    with open(source_file, "r") as f:
        contents = f.read()
        if not options.patterns:
            return json.loads(contents)
        for pattern in options.patterns:
            parts = pattern.split(":")
            contents = contents.replace("{%s}" % parts[0], parts[1])
        return json.loads(contents)

def _get_json_list(options):
    data_list = []
    #decode source file
    contents = __substitude_contents(options, options.source_file)
    if contents :
        data_list.append(contents)

    if options.extra_sandbox_cfgs is None:
        return data_list

    #decode extra file
    for sandbox_cfg in options.extra_sandbox_cfgs:
        contents = __substitude_contents(options, sandbox_cfg)
        if contents :
            data_list.append(contents)
    return data_list

def fix_sandbox_config_file(options):
    data_list = _get_json_list(options)
    #decode template
    origin_json = json.loads(APP_SANDBOX_DEFAULT)

    for data in data_list:
        # 处理common
        common = data.get("common")
        if common is not None and len(common) > 0:
            _merge_scope_common(origin_json.get("common")[0], common[0])

        #处理individual
        individuals = data.get("individual")
        if  individuals is not None and len(individuals) > 0:
            _merge_scope_individual(origin_json.get("individual")[0], individuals[0])
            pass
        
        # 处理permission
        permission = data.get("permission")
        if permission is not None and len(permission) > 0:
            _merge_scope_permission(origin_json.get("permission")[0], permission[0])

    # dump json to output
    flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
    modes = stat.S_IWUSR | stat.S_IRUSR | stat.S_IWGRP | stat.S_IRGRP
    with os.fdopen(os.open(options.output, flags, modes), 'w') as f:
        f.write(json.dumps(origin_json, ensure_ascii=False, indent=2))

def main(args):
    options = parse_args(args)
    depfile_deps = ([options.source_file])
    fix_sandbox_config_file(options)
    build_utils.write_depfile(options.depfile, options.output, depfile_deps, add_pydeps=False)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
