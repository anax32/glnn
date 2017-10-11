#include "gl\defs.h"
#include "gl\context.h"

#include <iostream>
#include <vector>

#include "glnnrt\gldnn_subtraction.h"
#include "test_bplate.h"

int main(int argc, char** argv)
{
    // create the tests
    test_set_t test_set(
    {
        { "subtraction_square", []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(8 * 8);
        std::vector<float>	B(8 * 8);
        std::vector<float>	output(8 * 8);

        std::fill(std::begin(A), std::end(A), 5.0f);
        std::fill(std::begin(B), std::end(B), 2.0f);
        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["A"], A, std::make_pair(8, 8));

        textures["B"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["B"], B, std::make_pair(8, 8));

        textures["output"] = gl::texture::create(std::make_pair(8, 8));

        {
            gldnn_subtraction	subtraction(
                textures["A"],
                textures["B"],
                textures["output"]);

            subtraction();
        }

        gl::texture::extract(textures["output"], output);

        return std::all_of(
            std::begin(output),
            std::end(output),
            [](const float x) {return x == 3.0f; });
    } },
    { "subtraction_non_square", []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(4 * 2);
        std::vector<float>	B(4 * 2);
        std::vector<float>	output(4 * 2);

        std::fill(std::begin(A), std::end(A), 5.0f);
        std::fill(std::begin(B), std::end(B), 2.0f);
        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(4, 2));
        gl::texture::set_content(textures["A"], A, std::make_pair(4, 2));

        textures["B"] = gl::texture::create(std::make_pair(4, 2));
        gl::texture::set_content(textures["B"], B, std::make_pair(4, 2));

        textures["output"] = gl::texture::create(std::make_pair(4, 2));

        {
            gldnn_subtraction	subtraction(
                textures["A"],
                textures["B"],
                textures["output"]);

            subtraction();
        }

        gl::texture::extract(textures["output"], output);

        return std::all_of(
            std::begin(output),
            std::end(output),
            [](const float x) {return x == 3.0f; });
    } },
    { "subtraction_square_counter", []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(8 * 8);
        std::vector<float>	B(8 * 8);
        std::vector<float>	output(8 * 8);
        std::vector<float>	exp(8 * 8);

        std::generate(
            std::begin(A),
            std::end(A),
            [i = 1.0f]() mutable {return i++; });

        std::fill(
            std::begin(B),
            std::end(B),
            2.0f);

        std::generate(
            std::begin(exp),
            std::end(exp),
            [i = 1.0f]() mutable {return i++ - 2.0f; });

        std::fill(std::begin(output), std::end(output), 0.0f);

        textures["A"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["A"], A, std::make_pair(8, 8));

        textures["B"] = gl::texture::create(std::make_pair(8, 8));
        gl::texture::set_content(textures["B"], B, std::make_pair(8, 8));

        textures["output"] = gl::texture::create(std::make_pair(8, 8));

        {
            gldnn_subtraction	subtraction(
                textures["A"],
                textures["B"],
                textures["output"]);

            subtraction();
        }

        gl::texture::extract(textures["output"], output);

        return std::equal(
            std::begin(output),
            std::end(output),
            std::begin(exp));
    } },
    { "subtraction_non_square_counter", []() -> bool {
        gl::texture::texture_set_t		textures;

        std::vector<float>	A(2 * 3);
        std::vector<float>	B(2 * 3);
        std::vector<float>	output(2 * 3);
        std::vector<float>  exp(2 * 3);

        std::generate(
            std::begin(A),
            std::end(A),
            [i = 1.0f]() mutable {return i++; });

        std::fill(
            std::begin(B),
            std::end(B),
            2.0f);

        std::fill(std::begin(output), std::end(output), 0.0f);

        std::generate(
            std::begin(exp),
            std::end(exp),
            [i = 1.0f]() mutable {return i++ - 2.0f; });

        textures["A"] = gl::texture::create(std::make_pair(2, 3));
        gl::texture::set_content(textures["A"], A, std::make_pair(2, 3));

        textures["B"] = gl::texture::create(std::make_pair(2, 3));
        gl::texture::set_content(textures["B"], B, std::make_pair(2, 3));

        textures["output"] = gl::texture::create(std::make_pair(2, 3));

        {
            gldnn_subtraction	subtraction(
                textures["A"],
                textures["B"],
                textures["output"]);

            subtraction();
        }

        gl::texture::extract(textures["output"], output);

        return std::equal(
            std::begin(output),
            std::end(output),
            std::begin(exp));
    } }
    });

    // create the gl context
    if (gl::context::create_windowless() == false)
    {
        std::cout << "gl::context::create failed" << std::endl;
        return 1;
    }

    // run the tests
    test_set.run();

    gl::context::clean(NULL);

    // output which tests failed
    test_set.report();

    // return the number of failures
    return test_set.fail_count();
}