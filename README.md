## Todo
* 引入Assimp库来加载mesh和材质资源，未来可以实现一个资产系统，并且实现一个转换器，将所有外部资产文件转换为内部的资产。
* 将资产的缓存从std::shared_ptr改为std::weak_ptr，这样就不会导致资产永远无法自动销毁。
* 改进渲染器renderer和场景scene的接口，为场景添加Light组件
* 实现PBR渲染。
* 更新fastgltf模块的代码，使用最新版本，并将其作为一个子模块。


## Todo for Rendering abstract interface
* 支持使用 timeline semaphore来同步 GPU-GPU 和 CPU-GPU。
* 设计一个类支持  queue up command lists, 并 batch submit.
* 实现begin renderpass 的接口