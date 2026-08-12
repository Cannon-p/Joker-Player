#pragma once
#include <fstream>
#include <chrono>

namespace aur
{
inline void traceStep (const char* s)
{
    std::ofstream out ("E:/My Plugin/juce_vscode/aur_trace.txt", std::ios::app);
    if (out.is_open())
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (now).count();
        out << ms << " " << s << "\n";
    }
}
} // namespace aur
