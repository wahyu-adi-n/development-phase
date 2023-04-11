
#include <cassert>
#include <iostream>
#include <fstream>

#include <torch/torch.h>
#include <torch/script.h>
#include <torch/nn/functional/activation.h>

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: <root-path> <torchscript-model-path> <input-data-path>" << std::endl;
        return -1;
    }

    torch::Device device = torch::kCPU;

    std::cout << "Loading model..." << std::endl;

    // load model
    torch::jit::script::Module module;
    try {
        module = torch::jit::load(argv[1], device);
    } catch (const c10::Error &e) {
        std::cerr << "Error loading model" << std::endl;
        std::cerr << e.what_without_backtrace() << std::endl;
        return -1;
    }

    std::cout << "Model loaded successfully" << std::endl;
    std::cout << std::endl;

    // switch off autograd, set evalation mode
    torch::NoGradGuard noGrad;
    module.eval();

    // read classes
    std::string line;
    std::ifstream ifsClasses("../class_names.txt", std::ios::in);
    if (!ifsClasses.is_open()) {
        std::cerr << "Cannot open class_names.txt" << std::endl;
        return -1;
    }
    std::vector<std::string> classes;
    while (std::getline(ifsClasses, line)) {
        classes.push_back(line);
    }
    ifsClasses.close();

    // read input
    std::ifstream ifsData(argv[2], std::ios::in | std::ios::binary);
    if (!ifsData.is_open()) {
        std::cerr << "Cannot open " << argv[2] << std::endl;
        return -1;
    }
    size_t size = 3 * 32 * 32 * sizeof(float);
    std::vector<char> data(size);
    ifsData.read(data.data(), data.size());
    ifsData.close();

    // create input tensor on CUDA device
    at::Tensor input = torch::from_blob(data.data(), {1, 3, 32, 32}, torch::kFloat);
    input = input.to(device);

    // create inputs
    std::vector<torch::jit::IValue> inputs{input};

    // execute model
    at::Tensor output = module.forward(inputs).toTensor();

    // apply softmax and get Top-5 results
    namespace F = torch::nn::functional;
    at::Tensor softmax = F::softmax(output, F::SoftmaxFuncOptions(1));
    std::tuple<at::Tensor, at::Tensor> top5 = softmax.topk(5);

    // get probabilities and labels
    at::Tensor probs = std::get<0>(top5);
    at::Tensor labels = std::get<1>(top5);

    // print probabilities and labels
    for (int i = 0; i < 5; i++) {
        float prob = 100.0f * probs[0][i].item<float>();
        long label = labels[0][i].item<long>();
        std::cout << std::fixed << std::setprecision(2) << prob << "% " << classes[label] << std::endl;
    }
    std::cout << std::endl;

    std::cout << "DONE" << std::endl;
    return 0;
}
