# EV_SDK

## 说明
### EV_SDK的目标
开发者专注于算法开发及优化，最小化业务层编码，即可快速部署到生产环境，共同打造商用级高质量算法。
### 极市平台做了哪些
1. 统一定义算法接口：针对万千视频和图片分析算法，抽象出接口，定义在`include`目录下的`ji.h`文件中
2. 提供工具包：比如算法授权（必须实现）、模型加密，在`3rd`目录下
3. 应用层服务：此模块不在ev_sdk中，比如视频处理服务、算法对外通讯的http服务等

### 开发者需要做什么
1. 模型的训练和调优
2. 实现`ji.h`约定的接口，同时包括授权、支持分析区域等功能
3. 实现约定的输入输出
4. 其他后面文档提到的功能

## 目录

### 代码目录结构

```
ev_sdk
|-- 3rd             # 第三方源码或库目录
|   |-- wkt_parser          # 针对使用WKT格式编写的字符串的解析器
|   |-- cJSON               # c版json库，使用内存管理较麻烦,容易造成内存泄露,主要是授权库对此有依赖
|   |-- jsoncpp_simple      # c++版json库，简单易用,推荐开发者在处理json使使用这个库
|   |-- fonts               # 图片可视化时使用的字体库
|   `-- license             # SDK授权库及相关工具
|-- CMakeLists.txt          # 本项目的cmake构建文件
|-- README.md       # 本说明文件
|-- model           # 模型数据存放文件夹
|-- config          # 程序配置目录
|   |-- README.md   # algo_config.json文件各个参数的说明和配置方法
|   `-- algo_config.json    # 程序配置文件
|-- doc
|-- include         # 库头文件目录
|   `-- ji.h        # libji.so的头文件，理论上仅有唯一一个头文件
|-- lib             # 本项目编译并安装之后，默认会将依赖的库放在该目录，包括libji.so,以及安装的第三方库
|-- src             # 实现ji.cpp的代码,算法实现相关的代码等
`-- test            # 针对ji.h中所定义接口的测试代码，请勿修改！！！
```
## <span id="jump">!!开发SDK的一般步骤(开发者重点关注的部分)</span>


1. 获取demo源码,将源码拷贝至目标路径/usr/local/ev_sdk（**注意！！！整个demo是属于一个git工程，避免将git工程的隐藏文件，如.git目录，放到目标路径/usr/local/ev_sdk中,因为ev_sdk目录也是一个git工程目录；建议是拷贝而不是直接把demo的目录名称重命名为ev_sdk而把原始ev_sdk覆盖掉或者把原始ev_sdk工程目录删掉；否则容易引起一些未知的错误**）,下载对应的模型到/usr/local/ev_sdk/model目录下并修改模型文件名.
   
```
   链接：https://pan.baidu.com/s/15IePTaiiqxPa7UKIQd2jFw 
   提取码：ev66   
   mDetector->Init("/usr/local/ev_sdk/model/vehicle_plate.onnx", mConfig.algoConfig.thresh);
```  
  
2. 生成授权相关文件,运行如下脚本,生成授权相关的公私钥和头文件，生成的公私钥文件会放在/usr/local/ev_sdk/authorization目录下,公私钥是算法授权验证的唯一凭证,同一个SDK在整个开发,优化迭代的周期中只能生成一次公私钥,中途不能更改.（**注意！！！以下命令在项目开发周期中，包括迭代优化，只需要执行一次，这样可以保持一致性，避免出现授权失败的问题**）
   ```shell
   # 生成公私钥以及公钥对应的头文件,整个项目周期里只需执行一遍
   bash /usr/local/ev_sdk/3rd/license/bin/oneKeyAuth.sh
   ```
3. 添加修改算法业务逻辑代码后编译ev_sdk，执行完成之后，`/usr/local/ev_sdk/lib`下将生成`libji.so`和相关的依赖库，以及`/usr/local/ev_sdk/bin/`下的测试程序`test-ji-api`。需要注意的是**一定要有install,目的是将相应地库都安装到ev_sdk/lib下面**
  ```shell
        mkdir -p /usr/local/ev_sdk/build
        cd /usr/local/ev_sdk/build
        cmake ..
        make install 
  ```
4. SDK运行时加载授权文件, 经过验证后的SDK才能正常运行算法.授权跟密钥以及机器的硬件信息绑定,所以同一个算法在同一台机器上生成的授权文件是不变的,我们提供以下工具生成授权文件.

    ```shell
        # 利用生成硬件参考码文件和授权文件,该工具会在/usr/local/ev_sdk/authorization/下生成硬件摘要reference.txt和授权文件license.txt
        bash /usr/local/ev_sdk/3rd/license/bin/oneKeyTest.sh 
    ```

5. 测试,我们提供了测试工具test-ji-api方便开发者进行算法的快速测试,在下面的[demo测试](#jump_dev_test)中我们将演示如何使用测试工具进行基本的测试(**这点非常重要,开发者需要先自测通过再提交算法**)
## demo说明
本项目基于极视平台EV_SDK3.0标准作为模板进行算法封装，旨在为开发者提供最直观的关于SDK3.0的理解. 本项目中本项目中我们利用**YOLOX目标检测框架,并采用Tensorrt进行模型推理**, 实现一个车辆和车牌检测算法, 算法逻辑为当车辆出现在指定ROI内部时触发报警,在算法返回图片及json中进行体现.本项目中算法首次运行时,先用Tensorrt直接加载onnx模型,并保存为trt模型,以便再次运行时直接trt模型,加快初始化.本项目中,我们使用了一般SDK开发中都会用的的配置解析,json字符串处理, 图片文字(框)绘制等功能,开发者可以参考demo中的部分逻辑.
### demo源码介绍
我们在demo开发中采用单一职责原则,将算法逻辑处理,模型推理,配置管理等功能封装到不同的C++类中,方便算法的开发和维护.
1. 在SampleAlgorithm.cpp文件中我们封装了算法逻辑,通过该对象的实现算法的初始化,调用,反初始化等功能.对象内部保存json字符串的成员变量,以及返回图片对象的cv::Mat成员变量,如果开发这按照如下的几个接口封装算法类,则无需对ji.cpp做任何更改.

    ```
    在ji.cpp ji_predictor_detector 中调用算法的初始化接口:        
        STATUS Init();

    在ji.cpp processMat 中调用算法分析接口和返回图片获取接口:
        STATUS Process(const Mat &inFrame, const char *args, JI_EVENT &event);      
        STATUS GetOutFrame(cv::Mat &out, unsigned int &outCount);
    
    在ji.cpp ji_destroy_detector 中调用算法反初始化接口:
        STATUS UnInit();               
    ```
2. 在Configuration.hpp文件中,我们封装了一个简单易用的配置管理工具,其中已经包含了一些常见的配置项,开发者需要根据算法需求自行增加或者删减配置项,下面演示如何基于配置管理类快速增加配置.假设我们需要增加一个nms阈值,只需在Configuration.hpp中增加如下两句,删除配置像亦然.
```
    struct Configuration
    {
        ..............
        float nms_thresh = 0.45; //增加成员
        ..............
        void ParseAndUpdateArgs(const char *confStr)
        {
            ..............
            checkAndUpdateNumber("nms_thresh", nms_thresh); //增加更新操作
            ..............
        }
        ..............
    }    
```

3. SampleDetector.cpp 中我们封装了一个基于tensorrt的YolovX目标检测类.
4. demo运行的基础镜像及容器启动命令
   
```
运行镜像环境
ccr.ccs.tencentyun.com/public_images/ubuntu16.04-cuda11.1-cudnn8.0-ffmpeg4.2-opencv4.1.2-tensorrt7.2.3-code-server3.5.0-ev_base:v1.0

容器运行命令
nvidia-docker run -itd --privileged ccr.ccs.tencentyun.com/public_images/ubuntu16.04-cuda11.1-cudnn8.0-ffmpeg4.2-opencv4.1.2-tensorrt7.2.3-code-server3.5.0-ev_base:v1.0 /bin/bash
```
### [demo测试](#jump_dev_test)
我们按照[开发SDK的一般步骤](#jump_dev)编译并授权过后即可运行测试工具,测试工具主要提供一下几个功能

```"shell"
---------------------------------
usage:
  -h  --help        show help information 显示帮助信息
  -f  --function    test function for 指定测试的功能
                    1.ji_calc_frame
                    2.ji_calc_buffer
                    3.ji_calc_file
                    4.ji_calc_video_file
                    5.ji_destroy_predictor
  -l  --license     license file. default: license.txt 指定授权文件路径
  -i  --infile      source file 输入图片视频的地址
  -a  --args        for example roi 指定参数
  -o  --outfile     result file 指定输出文件名称
  -r  --repeat      number of repetitions. default: 1  指定调用运行的次数
                    <= 0 represents an unlimited number of times
                    for example: -r 100
---------------------------------
```

下面我们对部分功能进行详细的说明(未说明的参数暂未实现)

1. 指定调用功能的-f参数
    1. -f 1/2/3指调用算法分析接口，调用该接口时主要支持如下几种输入方式:

    ```"shell"
        1.输入单张图片，需要指定输入输出文件
        　　./test-ji-api -l license.txt -f 1/2/3 -i ../data/persons.jpg -o result.jpg
                       
        2.-f 1 除了支持图片输入以外还支持视频输入，需要指定输入输出文件
        　　./test-ji-api -l license.txt -f 1 -i ../data/test.mp4 -o test_result.mp4         
    ```

    2. -f 4指调用视频分析接口，调用该接口时主要支持如下输入方式:

    ```"shell"
        1.输入视频文件，需要指定输入输出文件
        　　./test-ji-api -l license.txt -f 4 -i ../data/test.mp4 -o result.mp4
    ```

    3. -f 5指调用算法实例创建释放接口，该接口需要与-r参数配合使用，测试在循环创建/调用/释放的过程中是否存在内存/显存的泄露，与调用-f 1的区别在于，当-r参数指定调用次数时，-f 1只会创建一次实例，释放一次实例．

    ```'shell'
        ./test-ji-api -l license.txt  -f 5 -i ../data/persons.jpg -o result.jpg -r -1 #无限循环调用

        ./test-ji-api -l license.txt -f 5 -i ../data/persons.jpg -o result.jpg -r 100 #循环调用100次
    ```

2. 指定授权文件的路径-l参数，使用方式见上文介绍.
3. 指定输入的-i参数，使用方式见上文介绍.
4. 指定输出的-o参数，使用方式见上文介绍.
5. 指定调用次数的-r参数,使用方式见上文介绍.
6. 指定配置的-a参数,算法初始化时会从配置文件中加载默认配置参数,对于部分参数通过接口可以动态覆盖默认参数,如果项目要求能够动态指定的参数,需要测试通过-a传递的参数能够生效.例如,对于本demo的配置文件如下
   
   ```"json"
   {
    "draw_roi_area": true,    
    "roi_type": "polygon_1",
    "polygon_1": ["POLYGON((0 0, 1 0, 1 1, 0 1))"],
    "roi_color": [255, 255, 0, 0.7],
    "roi_line_thickness": 4,
    "roi_fill": false,
    "draw_result": true,
    "draw_confidence": true,
    "thresh": 0.1,
    "language": "en",
    "target_rect_color": [0, 0, 255, 0],
    "object_rect_line_thickness": 3,
    "object_text_color": [255, 255, 255, 0],
    "object_text_bg_color": [50, 50, 50, 0],
    "object_text_size": 30,
    "mark_text_en": ["vehicle", "plate"],
    "mark_text_zh": ["车辆","车牌"],
    "draw_warning_text": true,
    "warning_text_en": "WARNING! WARNING!",
    "warning_text_zh": "警告!",
    "warning_text_size": 30,
    "warning_text_color": [255, 255, 255, 0],
    "warning_text_bg_color": [0, 0, 200, 0],
    "warning_text_left_top": [0, 0]
    }
    ```

配置文件中的polygon_1参数和language参数需要支持动态配置,则需要利用-a参数测试
   
   ```
    //未指定参数
        ./test-ji-api -f 1 -i ../data/vp.jpeg -l ../authorization/license.txt -o result.jpg

    //指定参数
        ./test-ji-api -f 1 -i ../data/vp.jpeg  
        -a "{\"polygon_1\": [\"POLYGON((0.2 0.2, 0.8 0, 0.8 0.8, 0 0.8))\"],\"language\":\"zh\"}"
        -l ../authorization/license.txt -o result.jpg
   ```

以下为默认参数的输出效果  

![alt 默认参数](doc/figure1.jpg)

```
code: 0
        json: {
        "algorithm_data" : 
        {
                "is_alert" : true,
                "target_info" : 
                [
                        {
                                "confidence" : 0.92,
                                "height" : 54,
                                "name" : "vehicle",
                                "width" : 61,
                                "x" : 387,
                                "y" : 32
                        },
                        {
                                "confidence" : 0.96,
                                "height" : 16,
                                "name" : "plate",
                                "width" : 48,
                                "x" : 200,
                                "y" : 159
                        },
                        {
                                "confidence" : 0.97,
                                "height" : 152,
                                "name" : "vehicle",
                                "width" : 178,
                                "x" : 137,
                                "y" : 41
                        }
                ]
        },
        "model_data" : 
        {
                "objects" : 
                [
                        {
                                "confidence" : 0.92,
                                "height" : 54,
                                "name" : "vehicle",
                                "width" : 61,
                                "x" : 387,
                                "y" : 32
                        },
                        {
                                "confidence" : 0.96,
                                "height" : 16,
                                "name" : "plate",
                                "width" : 48,
                                "x" : 200,
                                "y" : 159
                        },
                        {
                                "confidence" : 0.97,
                                "height" : 152,
                                "name" : "vehicle",
                                "width" : 178,
                                "x" : 137,
                                "y" : 41
                        }
                ]
        }
    }
```
以下为指定参数的输出效果  

![alt 默认参数](doc/figure2.jpg)
```
code: 0
        json: {
        "algorithm_data" : 
        {
                "is_alert" : true,
                "target_info" : 
                [
                        {
                                "confidence" : 0.96,
                                "height" : 16,
                                "name" : "车牌",
                                "width" : 48,
                                "x" : 200,
                                "y" : 159
                        },
                        {
                                "confidence" : 0.97,
                                "height" : 152,
                                "name" : "车辆",
                                "width" : 178,
                                "x" : 137,
                                "y" : 41
                        }
                ]
        },
        "model_data" : 
        {
                "objects" : 
                [
                        {
                                "confidence" : 0.92,
                                "height" : 54,
                                "name" : "车辆",
                                "width" : 61,
                                "x" : 387,
                                "y" : 32
                        },
                        {
                                "confidence" : 0.96,
                                "height" : 16,
                                "name" : "车牌",
                                "width" : 48,
                                "x" : 200,
                                "y" : 159
                        },
                        {
                                "confidence" : 0.97,
                                "height" : 152,
                                "name" : "车辆",
                                "width" : 178,
                                "x" : 137,
                                "y" : 41
                        }
                ]
        }
}
```

## 规范要求

规范测试大部分内容依赖于内置的`/usr/local/ev_sdk/test`下面的代码，这个测试程序会链接`/usr/local/ev_sdk/lib/libji.so`库，`ev_SDK`封装完成提交后，极市方会使用`test-ji-api`程序测试`ji.h`中的接口。测试程序与`EV_SDK`的实现没有关系，所以请**请不要修改`/usr/local/ev_sdk/test`目录下的代码！！！**

1. 接口功能要求
  
   - 确定`test-ji-api`能够正常编译，并且将`test-ji-api`和`license.txt`移动到任意目录，都需要能够正常运行；
   
   - 在提交算法之前，请自行通过`/usr/local/ev_sdk/bin/test-ji-api`测试接口功能是否正常；
   
   - 未实现的接口需要返回`JISDK_RET_UNUSED`,大多数情况只需要实现接口`ji_calc_frame`即可,自测时测试该接口即可,输入文件为图片文件和视频文件均可；
   
   - 实现的接口，如果传入参数异常时，需要返回`JISDK_RET_INVALIDPARAMS`；
   
   - 输入图片和输出图片的尺寸应保持一致；
   
   - 对于接口中传入的参数`args`（如，`ji_calc_frame(void *, const JI_CV_FRAME *, const char *args, JI_CV_FRAME *, JI_EVENT *)`中中`args`），根据项目需求，算法实现需要支持`args`实际传入的参数。
   
     例如，如果项目需要支持在`args`中传入`roi`参数，使得算法只对`roi`区域进行分析，那么**算法内部必须实现只针对`roi`区域进行分析的功能**；
   
   - 通常输出图片中需要画`roi`区域、目标框等，请确保这一功能正常，包括但不仅限于：
   
     - `args`中输入的`roi`需要支持多边形
     - 算法默认分析区域必须是全尺寸图，如当`roi`传入为空时，算法对整张图进行分析；
     
   - 为了保证多个算法显示效果的一致性，与画框相关的功能必须优先使用`ji_utils.hpp.h`中提供的工具函数；
   
   > 1. ` test-ji-api`的使用方法可以参考上面的使用示例以及运行`test-ji-api --help`；
   > 2. 以上要求在示例程序`ji.cpp`中有实现；
   
2. 业务逻辑要求

   针对需要报警的需求，算法必须按照以下规范输出结果：
   * 报警时输出：`JI_EVENT.code=JISDK_CODE_ALARM`，`JI_EVENT.json`内部填充`"is_alert"=true`；
   * 未报警时输出：`JI_EVENT.code=JISDK_CODE_NORMAL`，`JI_EVENT.json`内部填充`"is_alert"=false`；
   * 处理失败的接口返回`JI_EVENT.code=JISDK_CODE_FAILED`

   * 算法输出的`json`数据必须与项目需求保持一致；

3. 授权功能要求

   需要实现授权功能，并且在调用接口（比如`ji_calc_frame`）时，如果授权没有通过，必须返回`JISDK_RET_UNAUTHORIZED`。

   > **注：授权功能已经在示例代码实现，基本不需要修改**

4. 算法配置选项要求

   - `EV_SDK`的实现需要使用标准[JSON](https://www.json.cn/wiki.html)格式的配置文件，所有算法与`SDK`可配置参数**必须**存放在统一的配置文件：`/usr/local/ev_sdk/config/algo_config.json`中；
   - 配置详细文档见config/README.md
   - **部分配置参数必须支持参数实时更新**。

5. 算法输出规范要求

   算法输出结果，即`JI_EVENT`必须是使用`json`格式填充的字符串，`json`字符串内所有的**键名称**必须是小写字母，并且单词之间使用下划线分隔，如`is_alert`；

6. 文件结构规范要求

   * 与模型相关的文件必须存放在`/usr/local/ev_sdk/model`目录下。
   * 最终编译生成的`libji.so`必须自行链接必要的库，`test-ji-api`不会链接除`/usr/local/ev_sdk/lib/libji.so`以外的算法依赖库；
   * 如果`libji.so`依赖了系统动态库搜索路径（如`/usr/lib`，`/lib`等）以外的库，必须将其安装到`/usr/local/ev_sdk/lib`下，可以使用`ldd /usr/local/ev_sdk/lib/libji.so`查看`libji.so`是否正确链接了所有的依赖库。
   * 第三方的依赖请放置在`/usr/local/ev_sdk/3rd`目录下,并且保证所有的库都最终能安装在`/usr/local/ev_sdk/lib`下,并且能够正常链接成功.
   * 注意**务必不要将运行时需要的文件或库放置在`/usr/local/ev_sdk/src`下面**,实际部署时该文件夹将被删除掉!!!


## FAQ

### jsoncpp中生成字符串的时候如何解决中文乱码和小数点精度问题？
```
    json::Value jv;
    jv['value'] = 0.1234567;
    jv['language'] = "中文";
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder.settings_["precision"] = 2;   //小数点后保留两位
    writerBuilder.settings_["emitUTF8"] = true; //utf8解决中文乱码问题
    std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    std::ostringstream os;
    jsonWriter->write(jRoot, &os);
    std::stringmStrOutJson = os.str();    //{"value":0.12,"language":"中文"}
```
### 为什么不能且不需要修改`/usr/local/ev_sdk/test`下的代码？

1. `/usr/local/ev_sdk/test`下的代码是用于测试`ji.h`接口在`libji.so`中是否被正确实现，这一测试程序与`EV_SDK`的实现无关，且是极市方的测试标准，不能变动；
2. 编译后`test-ji-api`程序只会依赖`libji.so`，如果`test-ji-api`无法正常运行，很可能是`libji.so`没有按照规范进行封装；

### 为什么运行`test-ji-api`时，会提示找不到链接库？

由于`test-ji-api`对于算法而言，只链接了`/usr/local/ev_sdk/lib/libji.so`库，如果`test-ji-api`运行过程中，找不到某些库，那么很可能是`libji.so`依赖的某些库找不到了。此时

1. 可以使用`ldd /usr/local/ev_sdk/lib/libji.so`检查是否所有链接库都可以找到；
2. 请按照规范将系统动态库搜索路径以外的库放在`/usr/local/ev_sdk/lib`目录下。

### 如何使用`test-ji-api`进行测试？

1. 输入单张图片和授权文件，并调用`ji_calc_frame`接口：

   ```shell
   ./test-ji-api -f ji_calc_frame -i /path/to/test.jpg -l /path/to/license.txt
   ```

2. 输入`json`格式的`roi`参数到`args`参数：

   ```shell
   ./test-ji-api \
   -f ji_calc_frame \
   -i /path/to/test.jpg \
   -l /path/to/license.txt \
   -a "{\"polygon_1\":[\"POLYGON((0.21666666666666667 0.255,0.6924242424242424 0.1375,0.8833333333333333 0.72,0.4106060606060606 0.965,0.048484848484848485 0.82,0.2196969696969697 0.2575))\"]}"
   ```

3. 保存输出图片：

   ```shell
   ./test-ji-api -f ji_calc_frame -i /path/to/test.jpg -l /path/to/license.txt -o /path/to/out.jpg
   ```

更多选项，请参考`test-ji-api --help`

### 如何使用jsoncpp生成和解析json字符串？
参见src/SampleAlgorithm.cpp中的使用示例