#ifndef CIFAR10_LABEL_H_WAN
#define CIFAR10_LABEL_H_WAN

#include <iostream>
#include <map>
namespace wan::labels
{
    extern std::map<int, std::string> cifar10_labels;

    std::map<int, std::string> cifar10_labels =
        {
            {0, "airplane"},
            {1, "automobile"},
            {2, "bird"},
            {3, "cat"},
            {4, "deer"},
            {5, "dog"},
            {6, "frog"},
            {7, "horse"},
            {8, "ship"},
            {9, "truck"}};
}
#endif
