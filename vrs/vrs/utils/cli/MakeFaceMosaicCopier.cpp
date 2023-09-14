/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
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

#include "MakeFaceMosaicCopier.h"
#include "opencv2/opencv.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <algorithm>

const std::map<std::string, int> str2backend{
    {"opencv", cv::dnn::DNN_BACKEND_OPENCV}, {"cuda", cv::dnn::DNN_BACKEND_CUDA},
    {"timvx",  cv::dnn::DNN_BACKEND_TIMVX},  {"cann", cv::dnn::DNN_BACKEND_CANN}
};
const std::map<std::string, int> str2target{
    {"cpu", cv::dnn::DNN_TARGET_CPU}, {"cuda", cv::dnn::DNN_TARGET_CUDA},
    {"npu", cv::dnn::DNN_TARGET_NPU}, {"cuda_fp16", cv::dnn::DNN_TARGET_CUDA_FP16}
};

class FaceMosaicer
{
public:
    FaceMosaicer(
          const std::string& model_path = "/usr/local/share/opencv4/face_detection_yunet_2023mar.onnx",
          const cv::Size& input_size = cv::Size(320, 320),
          float conf_threshold = 0.5f,
          float nms_threshold = 0.3f,
          int top_k = 20,
          const std::string& backend = "cuda",
          const std::string& target = "cuda")
        : model_path_(model_path), input_size_(input_size),
          conf_threshold_(conf_threshold), nms_threshold_(nms_threshold),
          top_k_(top_k), backend_id_(str2backend.at(backend)), target_id_(str2target.at(target))
    {
        model = cv::FaceDetectorYN::create(model_path_, "", input_size_, conf_threshold_, nms_threshold_, top_k_, backend_id_, target_id_);
    }

    void setBackendAndTarget(int backend_id, int target_id)
    {
        backend_id_ = backend_id;
        target_id_ = target_id;
        model = cv::FaceDetectorYN::create(model_path_, "", input_size_, conf_threshold_, nms_threshold_, top_k_, backend_id_, target_id_);
    }

    /* Overwrite the input size when creating the model. Size format: [Width, Height].
    */
    void setInputSize(const cv::Size& input_size)
    {
        input_size_ = input_size;
        model->setInputSize(input_size_);
    }

    void mosaic(const cv::Mat& image)
    {
        cv::Mat faces;
        model->detect(image, faces);
        for (int i = 0; i < faces.rows; ++i) {
            int x = static_cast<int>(faces.at<float>(i, 0));
            int y = static_cast<int>(faces.at<float>(i, 1));
            int w = static_cast<int>(faces.at<float>(i, 2));
            int h = static_cast<int>(faces.at<float>(i, 3));
            
            // Clamp roi coordinates
            x = std::max(0, x);
            y = std::max(0, y);
            w = std::min(image.cols - x, w);
            h = std::min(image.rows - y, h);
            cv::Point center(x + w / 2, y + h / 2);
            cv::Size axes(w / 3, h / 3);
            cv::ellipse(image, center, axes, 0, 0, 360, cv::Scalar(128, 128, 128), -1);
        }
    }

private:
    cv::Ptr<cv::FaceDetectorYN> model;

    std::string model_path_;
    cv::Size input_size_;
    float conf_threshold_;
    float nms_threshold_;
    int top_k_;
    int backend_id_;
    int target_id_;
};

namespace vrs::utils {

namespace {

class FaceMosaic : public RecordFilterCopier {
 public:
  FaceMosaic(
      vrs::RecordFileReader& fileReader,
      vrs::RecordFileWriter& fileWriter,
      vrs::StreamId id,
      const CopyOptions& copyOptions)
      : RecordFilterCopier(fileReader, fileWriter, id, copyOptions) {
        model = std::make_shared<FaceMosaicer>();
      }

  bool shouldCopyVerbatim(const CurrentRecord& record) override {
    auto tupleId = tuple<Record::Type, uint32_t>(record.recordType, record.formatVersion);
    const auto& verbatimCopyIter = verbatimCopy_.find(tupleId);
    if (verbatimCopyIter != verbatimCopy_.end()) {
      return verbatimCopyIter->second;
    }
    const auto& decoders = readers_.find(tuple<StreamId, Record::Type, uint32_t>(
        record.streamId, record.recordType, record.formatVersion));
    bool verbatimCopy = decoders == readers_.end() ||
        (record.recordType == Record::Type::DATA &&
         decoders->second.recordFormat.getBlocksOfTypeCount(ContentType::IMAGE) == 0 &&
         decoders->second.recordFormat.getBlocksOfTypeCount(ContentType::AUDIO) == 0);
    verbatimCopy_[tupleId] = verbatimCopy;
    return verbatimCopy;
  }
  void filterImage(const CurrentRecord& rec, size_t, const ContentBlock& ib, vector<uint8_t>& pixels)
      override {
    if (!pixels.empty()) {
      cv::Mat image = cv::imdecode(pixels, cv::IMREAD_COLOR);
      cv::transpose(image, image);
      cv::flip(image, image, 1);
      model->setInputSize(image.size());
      model->mosaic(image);
      cv::transpose(image, image);
      cv::flip(image, image, 0);
      cv::imencode(".jpg", image, pixels);
    }
  }
  void filterAudio(const CurrentRecord&, size_t, const ContentBlock&, vector<uint8_t>& audioSamples)
      override {
    if (!audioSamples.empty()) {
      memset(audioSamples.data(), 0, audioSamples.size());
    }
  }
 private:
    std::shared_ptr<FaceMosaicer> model;
 protected:
  map<tuple<Record::Type, uint32_t>, bool> verbatimCopy_;
};

} // namespace

unique_ptr<StreamPlayer> makeFaceMosaicCopier(
    RecordFileReader& fileReader,
    RecordFileWriter& fileWriter,
    StreamId streamId,
    const CopyOptions& copyOptions) {
  return std::make_unique<FaceMosaic>(fileReader, fileWriter, streamId, copyOptions);
}

} // namespace vrs::utils
