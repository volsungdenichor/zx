#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "zx/ansi/widgets/label.hpp"
#include "zx/ansi/widgets/layout.hpp"
#include "zx/app.hpp"
#include "zx/format.hpp"
#include "zx/functional.hpp"
#include "zx/image.hpp"
#include "zx/maybe.hpp"
#include "zx/string.hpp"
#include "zx/widget.hpp"

//  bazel run //:devlab && wslview ~/out.bmp
void run(const std::vector<std::string_view>&)
{
    const auto hippie = zx::mat::load_bitmap("/home/krzysiek/hippie.bmp");
    const auto conan = zx::mat::load_bitmap("/home/krzysiek/conan.bmp");
    std::cout << "hippie: " << hippie.extent() << std::endl;
    std::cout << "conan: " << conan.extent() << std::endl;

    auto out = hippie;

    zx::mat::copy(out.mut_view(), zx::mat::flip_horizontal(conan.view()), { 400, 300, 0 });

    zx::mat::save_bitmap(out, "/home/krzysiek/out.bmp");
}

void handle_exception(std::exception_ptr ptr, int level = 0)
{
    if (!ptr)
    {
        return;
    }

    try
    {
        std::rethrow_exception(ptr);
    }
    catch (const std::exception& ex)
    {
        std::cerr << std::string(level * 2, ' ') << ex.what() << std::endl;
        try
        {
            std::rethrow_if_nested(ex);
        }
        catch (...)
        {
            handle_exception(std::current_exception(), level + 1);
        }
    }
    catch (const std::string& ex)
    {
        std::cerr << std::string(level * 2, ' ') << ex << std::endl;
    }
    catch (const char* ex)
    {
        std::cerr << std::string(level * 2, ' ') << ex << std::endl;
    }
    catch (...)
    {
        std::cerr << std::string(level * 2, ' ') << "unknown exception" << std::endl;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        run(std::vector<std::string_view>(argv, argv + argc));
        return 0;
    }
    catch (...)
    {
        handle_exception(std::current_exception());
        return -1;
    }
}
