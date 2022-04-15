/*
 * Copyright (c) 2021 ExtremeVision Co., Ltd.
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


/**
 * 示例代码：实现ji.h定义的sdk接口，
 *  开发者只需要对本示例中的少量代码进行修改即可:
 *      SampleAlgorithm.hpp-----修改算法实现的头文件名
 *      SampleAlgorithm---------修改算法实现的类型名称
 * 请先浏览本demo示例的注释，帮助快速理解和进行开发
 */

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <glog/logging.h>
#include "ji_license.h"
#include "ji_license_impl.h"
#include "ji_utils.h"
#include "ji.h"
#include "pubKey.hpp"
#include "SampleAlgorithm.hpp"
// 如果需要添加授权功能，请保留该宏定义，并在ji_init中实现授权校验
#define ENABLE_JI_AUTHORIZATION


/*****************************************************************************************************************************************************************/
/****************************************************以下函数的实现可能需要根据算法实例的实现做少量修改*******************************************************************/
/**
 * @brief 使用predictor对输入图像inFrame进行处理 *
 * @param[in] predictor 算法实例的指针
 * @param[in] inFrame 输入图像
 * @param[in] args 处理当前输入图像所需要的输入参数，例如在目标检测中，通常需要输入ROI，由开发者自行定义和解析
 * @param[out] outFrame 输入图像，由内部填充结果，外部代码需要负责释放其内存空间
 * @param[out] event 以JI_EVENT封装的处理结果
 * @return 如果处理成功，返回JISDK_RET_SUCCEED
 */
int processMat(SampleAlgorithm *detector, const cv::Mat &inFrame, const char* args, cv::Mat &outFrame, JI_EVENT &event) {
    // 处理输入图像
    if (inFrame.empty()) 
    {
        return JISDK_RET_FAILED;
    }
#ifdef ENABLE_JI_AUTHORIZATION
    // 检查授权，统计QPS
    int ret = ji_check_expire();
    if (ret != JISDK_RET_SUCCEED) 
    {
        switch (ret) 
        {
            case EV_OVERMAXQPS:
                return JISDK_RET_OVERMAXQPS;
                break;
            case EV_OFFLINE:
                return JISDK_RET_OFFLINE;
                break;
            default:
                return JISDK_RET_UNAUTHORIZED;
        }
    }
#endif    
    //调用算法进行分析    
    STATUS processRet = detector->Process(inFrame, args, event);
    unsigned int count;
    detector->GetOutFrame(outFrame, count);
    return JISDK_RET_SUCCEED;
}


/**
 * @brief 调用此接口创建一个算法对象的实例,并返回指向算法实例的指针 
 * @param[in] pdtype 指示算法类型的参数,暂时未使用
 * @return 返回算法实例的指针
 */
void *ji_create_predictor(int pdtype) 
{
#ifdef ENABLE_JI_AUTHORIZATION
    if (ji_check_expire_only() != EV_SUCCESS) 
    {
        return nullptr;
    }
#endif
    auto *detector = new SampleAlgorithm;
    //调用算法的资源初始化函数
    if (detector->Init() != SampleAlgorithm::STATUS_SUCCESS) 
    {
        delete detector;
        detector = nullptr;
        SDKLOG(ERROR) << "Predictor init failed.";
        return nullptr;
    }
    SDKLOG(INFO) << "SamplePredictor init OK.";
    return detector;    
}


/**
 * @brief 调用此接口销毁一个算法实例,每次调用ji_create_predictor创建的算法实例都必须调用该接口来销毁
 * @param[in] predictor 需要销毁的算法实例 
 */
void ji_destroy_predictor(void *predictor) 
{
    if (predictor == nullptr) return;
    auto *detector = reinterpret_cast<SampleAlgorithm *>(predictor);
    //调用反初始化函数,释放算法实例的资源
    detector->UnInit();
    delete detector;
    detector = nullptr;
}

/*****************************************************************************************************************************************************************/
/******************************************************************************以下函数不需要修改********************************************************************/

/**
 * @brief 调用算法之前必须调用的初始化接口,进行授权的校验,开发者不能修改此函数的内容 
 */
int ji_init(int argc, char **argv) 
{
    SDKLOG(INFO) << "EV_SDK version:" << EV_SDK_VERSION;
    SDKLOG(INFO) << "License version:" << EV_LICENSE_VERSION;
    int authCode = JISDK_RET_SUCCEED;
#ifdef ENABLE_JI_AUTHORIZATION
    // 检查license参数
    if (argc < 6) 
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    if (argv[0] == NULL || argv[5] == NULL) 
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    int qps = 0;
    if (argv[4]) qps = atoi(argv[4]);

    // 使用公钥校验授权信息
    int ret = ji_check_license(pubKey, argv[0], argv[1], argv[2], argv[3], qps > 0 ? &qps : NULL, atoi(argv[5]));
    if (ret != EV_SUCCESS) 
    {
        authCode = JISDK_RET_UNAUTHORIZED;
    }
#endif
    if (authCode != JISDK_RET_SUCCEED) 
    {
        SDKLOG(ERROR) << "ji_check_license failed!";
        return authCode;
    }
    return authCode;
}

void ji_reinit() 
{
#ifdef ENABLE_JI_AUTHORIZATION
    ji_check_license(NULL, NULL, NULL, NULL, NULL, NULL, 0);
#endif    
}



int ji_calc_frame(void *predictor, const JI_CV_FRAME *inFrame, const char *args,
                  JI_CV_FRAME *outFrame, JI_EVENT *event) {
    if (predictor == NULL || inFrame == NULL) {
        return JISDK_RET_INVALIDPARAMS;
    }

    auto *detector = reinterpret_cast<SampleAlgorithm *>(predictor);
    cv::Mat inMat(inFrame->rows, inFrame->cols, inFrame->type, inFrame->data, inFrame->step);
    if (inMat.empty()) {
        return JISDK_RET_FAILED;
    }
    cv::Mat outputFrame;
    int processRet = processMat(detector, inMat, args, outputFrame, *event);

    if (processRet == JISDK_RET_SUCCEED) {
        if ((event->code != JISDK_CODE_FAILED) && (!outputFrame.empty()) && (outFrame)) {
            outFrame->rows = outputFrame.rows;
            outFrame->cols = outputFrame.cols;
            outFrame->type = outputFrame.type();
            outFrame->data = outputFrame.data;
            outFrame->step = outputFrame.step;
        }
    }
    return processRet;
}

int ji_calc_buffer(void *predictor, const void *buffer, int length, const char *args, const char *outFile,
                   JI_EVENT *event) {
    if (predictor == NULL || buffer == NULL || length <= 0) {
        return JISDK_RET_INVALIDPARAMS;
    }

    auto *detector = reinterpret_cast<SampleAlgorithm *>(predictor);

    const unsigned char *b = (const unsigned char *) buffer;
    std::vector<unsigned char> vecBuffer(b, b + length);
    cv::Mat inMat = cv::imdecode(vecBuffer, cv::IMREAD_COLOR);
    if (inMat.empty()) {
        return JISDK_RET_FAILED;
    }

    cv::Mat outMat;
    int processRet = processMat(detector, inMat, args, outMat, *event);

    if (processRet == JISDK_RET_SUCCEED) {
        if ((event->code != JISDK_CODE_FAILED) && (!outMat.empty()) && (outFile)) {
            cv::imwrite(outFile,outMat);
        }
    }
    return processRet;
}

int ji_calc_file(void *predictor, const char *inFile, const char *args, const char *outFile, JI_EVENT *event) {
    if (predictor == NULL || inFile == NULL) {
        return JISDK_RET_INVALIDPARAMS;
    }
    auto *detector = reinterpret_cast<SampleAlgorithm *>(predictor);
    cv::Mat inMat = cv::imread(inFile);
    if (inMat.empty()) {
        return JISDK_RET_FAILED;
    }

    cv::Mat outMat;
    int processRet = processMat(detector, inMat, args, outMat, *event);
    if (processRet == JISDK_RET_SUCCEED) {
        if ((event->code != JISDK_CODE_FAILED) && (!outMat.empty()) && (outFile)) {
            cv::imwrite(outFile, outMat);
        }
    }
    return processRet;
}

int ji_calc_video_file(void *predictor, const char *infile, const char* args,
                       const char *outfile, const char *jsonfile) {
    // 没有实现的接口必须返回`JISDK_RET_UNUSED`
    if (predictor == NULL || infile == NULL) {
        return JISDK_RET_INVALIDPARAMS;
    }
    auto *detector = reinterpret_cast<SampleAlgorithm *>(predictor);

    cv::VideoCapture videoCapture(infile);
    if (!videoCapture.isOpened()) {
        return JISDK_RET_FAILED;
    }

    cv::VideoWriter vwriter;
    cv::Mat inMat, outMat;
    JI_EVENT event;
    int iRet = JISDK_RET_FAILED;
    int totalFrames, alertFrames, timestamp;
    totalFrames = alertFrames = timestamp = 0;

    Json::Value jsonRoot;
    jsonRoot["detail"].resize(0);

    while (videoCapture.read(inMat)) {
        timestamp = videoCapture.get(cv::CAP_PROP_POS_MSEC);

        iRet = processMat(detector, inMat, args, outMat, event);

        if (iRet == JISDK_RET_SUCCEED) {
            ++totalFrames;

            if (event.code != JISDK_CODE_FAILED) {
                if (event.code == JISDK_CODE_ALARM) {
                    ++alertFrames;
                }

                if (!outMat.empty() && outfile) {
                    if (!vwriter.isOpened()) {
                        vwriter.open(outfile, cv::VideoWriter::fourcc('X', '2', '6', '4'), videoCapture.get(cv::CAP_PROP_FPS), outMat.size());
                        if (!vwriter.isOpened()) {
                            return JISDK_RET_FAILED;
                        }
                    }
                    vwriter.write(outMat);
                }

                if (event.json && jsonfile) {                    
                    Json::Value tmp;
                    tmp["timestamp"] = timestamp;
                    jsonRoot["detail"].append(tmp);                    
                }
            }
        } else {
            break;
        }
    }

    if (iRet == JISDK_RET_SUCCEED) {
        if (jsonfile) {            
            jsonRoot["total_frames"] = totalFrames;
            jsonRoot["alert_frames"] = alertFrames;                        
            std::ofstream fs(jsonfile);
            if (fs.is_open()) {
                Json::StreamWriterBuilder writerBuilder;
                // writerBuilder.settings_["precision"] = 2;
                writerBuilder.settings_["emitUTF8"] = true;    
                std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
                std::ostringstream os;
                jsonWriter->write(jsonRoot, &os);                
                fs << os.str();
                fs.close();
            }            
        }
    }
    return iRet;
}