#include "shader.hpp"

#include <fstream>
#include <iostream>


Shader::Shader(const GLchar *compute_path)
{
    std::ifstream compute_ifs(compute_path);
    std::string compute_code((std::istreambuf_iterator<char>(compute_ifs)),
                             (std::istreambuf_iterator<char>()));
	const GLchar *compute_code_cstr = compute_code.c_str();

	GLuint compute_shader;
	compute_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute_shader, 1, &compute_code_cstr, NULL);
	glCompileShader(compute_shader);
	checkShaderErrors(compute_shader);

	_program = glCreateProgram();
	glAttachShader(_program, compute_shader);
	glLinkProgram(_program);
	checkProgramErrors(_program);

	glDeleteShader(compute_shader);
}

Shader::Shader(const GLchar *vertex_path, const GLchar *fragment_path)
{
    std::ifstream vertex_ifs(vertex_path);
    std::string vertex_code((std::istreambuf_iterator<char>(vertex_ifs)),
                            (std::istreambuf_iterator<char>()));
	const GLchar *vertex_code_cstr = vertex_code.c_str();

    std::ifstream fragment_ifs(fragment_path);
    std::string fragment_code((std::istreambuf_iterator<char>(fragment_ifs)),
                              (std::istreambuf_iterator<char>()));
	const GLchar *fragment_code_cstr = fragment_code.c_str();

	GLuint vertex_shader, fragment_shader;
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_code_cstr, NULL);
	glCompileShader(vertex_shader);
    checkShaderErrors(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_code_cstr, NULL);
	glCompileShader(fragment_shader);
	checkShaderErrors(fragment_shader);

    _program = glCreateProgram();
	glAttachShader(_program, vertex_shader);
	glAttachShader(_program, fragment_shader);
	glLinkProgram(_program);
    checkProgramErrors(_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

void
Shader::use()
{
	glUseProgram(_program);
}

void
Shader::checkShaderErrors(GLuint shader)
{
	GLint success;
	GLchar info_log[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, info_log);
        std::cout << info_log << "\n\n\n";
	}
}

void
Shader::checkProgramErrors(GLuint program)
{
    GLint success;
	GLchar info_log[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, info_log);
        std::cout << info_log << "\n\n\n";
	}
}
