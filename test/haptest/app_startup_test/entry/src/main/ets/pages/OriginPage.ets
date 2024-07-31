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

import { Constants } from '../common/Constants';

@Component
export struct OriginPage {
  @Consume('navPathStack') navPathStack: NavPathStack;

  build() {
    NavDestination() {
      Column() {
        Button($r('app.string.back_to_web_page'))
          .width(Constants.FULL_SCREEN)
          .height($r('app.float.button_height'))
          .onClick(() => {
            this.navPathStack.pop();
          })
      }
      .width(Constants.FULL_SCREEN)
      .height(Constants.FULL_SCREEN)
      .justifyContent(FlexAlign.End)
      .padding({
        left: $r('app.float.space_24'),
        right: $r('app.float.space_24'),
        bottom: $r('app.float.space_bottom')
      })
    }
    .title(Constants.ORIGIN_PAGE_CHINESE)
  }
}