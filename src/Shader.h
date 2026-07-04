#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath) {
        const std::string vertSrc = readFile(vertexPath);
        const std::string fragSrc = readFile(fragmentPath);

        const unsigned int vert = compile(GL_VERTEX_SHADER,   vertSrc.c_str(), "VERTEX");
        const unsigned int frag = compile(GL_FRAGMENT_SHADER, fragSrc.c_str(), "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);
        checkLink(ID);

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() const { glUseProgram(ID); }

    void setMat4(const char* name, const glm::mat4& m) const {
        glUniformMatrix4fv(loc(name), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const char* name, const glm::vec3& v) const {
        glUniform3fv(loc(name), 1, glm::value_ptr(v));
    }
    void setFloat(const char* name, float v) const {
        glUniform1f(loc(name), v);
    }
    void setVec2(const char* name, const glm::vec2& v) const {
        glUniform2fv(loc(name), 1, glm::value_ptr(v));
    }
    void setVec4(const char* name, const glm::vec4& v) const {
        glUniform4fv(loc(name), 1, glm::value_ptr(v));
    }

private:
    GLint loc(const char* name) const {
        return glGetUniformLocation(ID, name);
    }

    static std::string readFile(const char* path) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        } catch (const std::ifstream::failure& e) {
            std::cerr << "[Shader] Cannot open file: " << path
                      << "  (" << e.what() << ")\n";
            return "";
        }
    }

    static unsigned int compile(GLenum type, const char* src, const char* label) {
        unsigned int id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int ok;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(id, sizeof(log), nullptr, log);
            std::cerr << "[Shader] Compile error (" << label << "):\n" << log << "\n";
        }
        return id;
    }

    static void checkLink(unsigned int prog) {
        int ok;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
            std::cerr << "[Shader] Link error:\n" << log << "\n";
        }
    }
};
