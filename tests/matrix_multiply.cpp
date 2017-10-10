#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glew32s.lib")

#include <Windows.h>
#include <wingdi.h>

#include "gl\defs.h"
#include "gl\context.h"

#include <iostream>
#include <vector>

#include "glnnrt\gldnn_multiply.h"
#include "test_bplate.h"

int main(int argc, char** argv)
{
    test_set_t test_set;

    // create the tests
    test_set.tests["square_mult_correct"] = []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(8 * 8);
        std::vector<float>	B(8 * 8);
        std::vector<float>	output(8 * 8);

        std::fill(std::begin(A), std::end(A), 1.0f);
        std::fill(std::begin(B), std::end(B), 1.0f);
        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["A"], A, std::make_pair(8, 8));

        textures["B"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["B"], B, std::make_pair(8, 8));

        textures["output"] = gl::texture::create(std::make_pair(8, 8));

        {
            gldnn_multiply	multiply(
                textures["A"],
                textures["B"],
                textures["output"]);

            multiply();
        }

        gl::texture::extract(textures["output"], output);

        return std::all_of(
            std::begin(output),
            std::end(output),
            [](const float x) {return x == 8.0f; });
    };
    test_set.tests["non_square_mult_correct"] = []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(4 * 2);
        std::vector<float>	B(2 * 3);
        std::vector<float>	output(4 * 3);

        std::fill(std::begin(A), std::end(A), 1.0f);
        std::fill(std::begin(B), std::end(B), 1.0f);
        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(4, 2));
        gl::texture::set_content(textures["A"], A, std::make_pair(4, 2));

        textures["B"] = gl::texture::create(std::make_pair(2, 3));
        gl::texture::set_content(textures["B"], B, std::make_pair(2, 3));

        textures["output"] = gl::texture::create(std::make_pair(4, 3));

        {
            gldnn_multiply	multiply(
                textures["A"],
                textures["B"],
                textures["output"]);

            multiply();
        }

        gl::texture::extract(textures["output"], output);

        return std::all_of(
            std::begin(output),
            std::end(output),
            [](const float x) {return x == 2.0f; });
    };
    test_set.tests["square_mult_counter_correct"] = []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(8 * 8);
        std::vector<float>	B(8 * 8);
        std::vector<float>	output(8 * 8);

        std::vector<float>  exp{
             1380,  1416,  1452,  1488,  1524,  1560,  1596,  1632,
             3236,  3336,  3436,  3536,  3636,  3736,  3836,  3936,
             5092,  5256,  5420,  5584,  5748,  5912,  6076,  6240,
             6948,  7176,  7404,  7632,  7860,  8088,  8316,  8544,
             8804,  9096,  9388,  9680,  9972, 10264, 10556, 10848,
            10660, 11016, 11372, 11728, 12084, 12440, 12796, 13152,
            12516, 12936, 13356, 13776, 14196, 14616, 15036, 15456,
            14372, 14856, 15340, 15824, 16308, 16792, 17276, 17760
        };

        std::generate(
            std::begin(A),
            std::end(A),
            [i = 1.0f]() mutable {return i++; });

        std::generate(
            std::begin(B),
            std::end(B),
            [i = 1.0f]() mutable {return i++; });

        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["A"], A, std::make_pair(8, 8));

        textures["B"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["B"], B, std::make_pair(8, 8));

        textures["output"] = gl::texture::create(std::make_pair(8, 8));

        {
            gldnn_multiply	multiply(
                textures["A"],
                textures["B"],
                textures["output"]);

            multiply();
        }

        gl::texture::extract(textures["output"], output);

        return std::equal(
            std::begin(output),
            std::end(output),
            std::begin(exp));
    };
    test_set.tests["non_square_mult_counter_correct"] = []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(4 * 2);
        std::vector<float>	B(2 * 3);
        std::vector<float>	output(4 * 3);

        std::vector<float>  exp{
             9, 12, 14,
            19, 26, 33,
            29, 40, 51,
            39, 54, 69
        };

        std::generate(
            std::begin(A),
            std::end(A),
            [i = 1.0f]() mutable {return i++; });

        std::generate(
            std::begin(B),
            std::end(B),
            [i = 1.0f]() mutable {return i++; });

        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(4, 2));
        gl::texture::set_content(textures["A"], A, std::make_pair(4, 2));

        textures["B"] = gl::texture::create(std::make_pair(2, 3));
        gl::texture::set_content(textures["B"], B, std::make_pair(2, 3));

        textures["output"] = gl::texture::create(std::make_pair(4, 3));

        {
            gldnn_multiply	multiply(
                textures["A"],
                textures["B"],
                textures["output"]);

            multiply();
        }

        gl::texture::extract(textures["output"], output);

         return std::equal(
            std::begin(output),
            std::end(output),
            std::begin(exp));
    };

    // create the gl context
    if (gl::context::create_windowless() == false)
    {
        std::cout << "gl::context::create failed" << std::endl;
        return 1;
    }

    // run the tests
    test_set.run ();

    gl::context::clean(NULL);

    // output which tests failed
    test_set.report ();

    // return the number of failures
    return test_set.fail_count ();
}