#ifndef INCLUDE_WAN_ANALYTICS_H
#define INCLUDE_WAN_ANALYTICS_H

#include <libasyik/http.hpp>
#include <opencv2/opencv.hpp>
#include "../common.h"
#include "torch/torch.h"
#include "torch/script.h"

using json = nlohmann::json;
namespace wan::analytics
{
    std::vector<int> input_shape{1, 3, 32, 32};

    // Declaration
    class BaseService
    {
    public:
        virtual auto loadImageB64(const std::string &) -> torch::Tensor = 0;
        virtual auto preprocess(const torch::Tensor) -> torch::Tensor = 0;
        virtual auto predict(torch::Tensor) -> std::pair<std::string, float> = 0;
    };

    class ClassificationService : BaseService
    {
    private:
        std::string model_path;

    public:
        torch::jit::script::Module module;
        ClassificationService(std::string &);
        auto loadImageB64(const std::string &) -> torch::Tensor override;
        auto preprocess(const torch::Tensor) -> torch::Tensor override;
        auto predict(torch::Tensor) -> std::pair<std::string, float> override;
    };

    class BasePipeline
    {
    private:
        json response;

    public:
        virtual void routeResponse(asyik::http_request_ptr &req, asyik::http_route_args &args, const std::string &m, ClassificationService &classificationService)
        {
            this->response = {{"title", "Default"}, {"message", "Default Message"}};
            req->response.body = this->response.dump();
        }
        void invalidRoute(asyik::http_request_ptr &req, asyik::http_route_args &args, const std::string &m);
    };
    class ClassificationPipeline : public BasePipeline
    {
    private:
        json response;

    public:
        ClassificationPipeline();
        void routeResponse(asyik::http_request_ptr &req, asyik::http_route_args &args, const std::string &m, ClassificationService &classificationService) override;
    };

    // Definition
    ClassificationPipeline::ClassificationPipeline() {}

    void BasePipeline::invalidRoute(asyik::http_request_ptr &req, asyik::http_route_args &args, const std::string &m)
    {
        json response;
        response["title"] = "not_found_exception";
        response["message"] = "No route found for " + m + " " + args[0];
        // Set content-type to application/json to make the response behaves as a json
        req->response.headers.set("content-type", "application/json");
        req->response.result(404);
        req->response.body = response.dump(4);
        LOG(INFO) << " Response:\n"
                  << req->response.body << "\n";
    }
    void ClassificationPipeline::routeResponse(asyik::http_request_ptr &req, asyik::http_route_args &args, const std::string &m, ClassificationService &classificationService)
    {
        auto timeStart = std::chrono::system_clock::now();
        json payload;

        if (!req->body.empty())
        {
            LOG(INFO) << " Payload\n"
                      << req->body << '\n';
            payload = json::parse(req->body);
        }
        // Default response
        this->response["result"] = {
            {"predicted", nullptr}, 
            {"score", nullptr},     // Percentage score
            {"status", "failed"},
            {"message", "failed"}};
        req->response.result(500);
        // Set content-type to application/json to make the response behaves as a json
        req->response.headers.set("content-type", "application/json");
        req->response.headers.set("x-asyik-reply", "ok");

        torch::Tensor img = (!payload.empty() && payload.contains("image")) ? classificationService.loadImageB64(payload["image"]) : torch::Tensor();
        LOG(ERROR) << COLOR(red) << COND(!img.numel()) << "EMPTY TENSOR - COULD BE CAUSED BY WRONG INPUT" << COLOR(white) << "\n";
        if (img.numel())
        {
            auto preprocessed_img = classificationService.preprocess(img);
            auto results = classificationService.predict(preprocessed_img);
            response["result"] = {
                {"predicted", results.first},
                {"score", results.second},    // Percentage score
                {"status", "ok"},
                {"message", "success"}};
            req->response.result(200);
        }
        req->response.body = response.dump(4);
        LOG(INFO) << " Response:\n"
                  << req->response.body << "\n";
        std::chrono::duration<double> endTime = std::chrono::system_clock::now() - timeStart;
        LOG(INFO) << "End-to-end time : " << endTime.count() * 1000.0f << " ms\n";
    }

    ClassificationService::ClassificationService(std::string &path) : model_path(path)
    {
        try
        {
            this->module = torch::jit::load(this->model_path);
            this->module.eval();
        }
        catch (const c10::Error &e)
        {
            LOG(ERROR) << "error loading the model\n";
            LOG(ERROR) << e.msg() << "\n";
        }
    }

    /**
     * @brief
     * Load image from an encoded base64 string
     * @param base64
     * @return torch::Tensor
     */
    torch::Tensor ClassificationService::loadImageB64(const std::string &base64)
    {
        if (base64.empty())
            return torch::Tensor();
        std::istringstream f(base64);
        std::string tmp, processed;
        while (getline(f, tmp, ','))
        {
            processed = tmp;
        }
        std::string buffer;
        try
        {
            buffer = base64_decode(processed);
        }
        catch (const std::runtime_error &e)
        {
            LOG(ERROR) << COLOR(red) << e.what() << "\n"
                       << COLOR(white);
            return torch::Tensor();
        }
        LOG(DEBUG) << buffer << std::endl;
        std::vector<uchar> data(buffer.begin(), buffer.end());
        cv::Mat img = cv::imdecode(data, cv::IMREAD_UNCHANGED);
        // BGR to RGB due to PIL Library usage in PyTorch
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
        if (img.size().empty() || !img.data)
        {
            return torch::Tensor();
        }
        cv::resize(img, img, cv::Size(input_shape[3], input_shape[2]));
        // Image shape
        // (1, 224, 224, 3)
        // (1, H, W, C)
        auto image_data = torch::from_blob(img.data, {1, img.rows, img.cols, 3}, torch::kByte).clone();
        // Reshape to
        // (1, C, H, W)
        // (1, 3, 224, 224)
        image_data = image_data.permute({0, 3, 1, 2});
        return image_data;
    }

    /**
     * @brief
     * Preprocess the image data such as resizing the image to match input shape and normalizing the pixels value
     * @param img_data
     * @return torch::Tensor
     */
    torch::Tensor ClassificationService::preprocess(const torch::Tensor img_data)
    {
        std::vector<_Float32> mean_vec{0.485, 0.456, 0.406};
        std::vector<_Float32> stddev_vec{0.229, 0.224, 0.225};
        torch::Tensor norm_img_data = torch::zeros(img_data.sizes());

        norm_img_data[0][0] = img_data[0][0].div(255.0f).sub_(mean_vec[0]).div_(stddev_vec[0]);
        norm_img_data[0][1] = img_data[0][1].div(255.0f).sub_(mean_vec[1]).div_(stddev_vec[1]);
        norm_img_data[0][2] = img_data[0][2].div(255.0f).sub_(mean_vec[2]).div_(stddev_vec[2]);
        return norm_img_data;
    }

    /**
     * @brief
     * Inference function
     * @param data
     * Input tensor data
     * @param module
     * Traced script module
     * @return std::pair<std::string, float>
     * @example
     * // returns {"cat", 43.5}
     * inference(image, module)
     * @returns {predicted, score} return the predicted class and the probability score (%) of the iamge
     */
    std::pair<std::string, float> ClassificationService::predict(torch::Tensor data)
    {
        LOG(INFO) << "Input shape : " << data.sizes() << "\n";
        auto timeStart = std::chrono::system_clock::now();
        torch::Tensor outputs = module.forward({data}).toTensor();
        std::chrono::duration<double> endTime = std::chrono::system_clock::now() - timeStart;
        LOG(DEBUG) << "Output : " << std::endl;
        LOG(DEBUG) << outputs << std::endl;
        LOG(INFO) << "Inference time : " << endTime.count() * 1000.0f << " ms\n";
        auto results = outputs.softmax(-1);
        auto result = results.sort(-1, true);
        auto softmaxs = std::get<0>(result)[0];
        auto indexs = std::get<1>(result)[0];
        return {wan::labels::cifar10_labels[indexs[0].item<int>()], (softmaxs[0].item<float>() * 100.0f)};
    }

}

#endif
