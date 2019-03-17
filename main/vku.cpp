/*=======================================================================
This file is part of vkPlay

vkPlay is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

vkPlay is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
//=======================================================================
/*
Descriptions:

Notes:

Authors:
2KYan, 2KYan@outlook.com

*/
//=======================================================================
//

#if defined(WIN32)
#include <windows.h>
#elif defined (LINUX)
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <mutex>
#include <array>

#include "vku.h"
#include "shaderc/shaderc.hpp"

vku* vku::instance()
{
    std::mutex inst_m;
    std::lock_guard<std::mutex> l(inst_m);

    static std::unique_ptr<vku> thisPtr(new vku);
    static bool isInit = false;
    if (isInit == false) {
        isInit = true;
        thisPtr->initResPaths();
    }

    return thisPtr.get();
}

vku::vku() 
{
    m_enableSPVDump = false;
}

vku::~vku()
{
}

int vku::initResPaths()
{
    char* envPath = nullptr;
    std::vector<std::string> paths = { "./" };
    std::vector<std::string> resourceTypeStrings = { "shader/", "texture/" };

    std::unordered_map<std::string, std::string> envPaths = {
        { "DEV_HOME", "/data/"},
    };

    for (const auto& envVar : envPaths) {
        if ((envPath = getenv(envVar.first.c_str())) != nullptr) {
            paths.push_back(std::string(envPath) + envVar.second);
        }
    }

    std::string resPath;
    struct stat buffer;
    for (int i = 0; i < static_cast<int>(ResourceType::NUM_RESOURCES); ++i) {
        resPaths[i].clear();
        for (const auto& path : paths) {
                resPath = path;
            if (stat(resPath.c_str(), &buffer) == 0) {
                resPaths[i].push_back(resPath);
            }
            resPath = path + resourceTypeStrings[i];
            if (stat(resPath.c_str(), &buffer) == 0) {
                resPaths[i].push_back(resPath);
            }
        }
    }

    return 0;
}

int vku::numResPaths()
{
    int numPaths = 0;
    for (auto& p: resPaths) {
        numPaths += int(p.size());
    }
    return numPaths;
}

std::string vku::getShaderFileName(const char* fileName) {
    return getResourceFileName(fileName, ResourceType::SHADER);
}

std::string vku::getTextureFileName(const char* fileName) {
    return getResourceFileName(fileName, ResourceType::TEXTURE);
}

std::string vku::getResourceFileName(const std::string& fileName, ResourceType resType)
{
    struct stat buffer;
    for (const auto& resPath : resPaths[static_cast<int>(resType)]) {
        if (stat((resPath + fileName).c_str(), &buffer) == 0) {
            return resPath + fileName;
        }
    }

    return std::string();
}

std::string vku::getFileNameWoExt(const std::string& fileName)
{
    std::string fileNameStr(fileName);
    std::string sep("\\");
    size_t offset = 0;
    while ((offset = fileNameStr.find(sep, offset)) != std::string::npos) {
        fileNameStr.replace(offset, sep.length(), "/");
    }
    if ((offset = fileNameStr.rfind("/", fileNameStr.length())) != std::string::npos) {
        fileNameStr.erase(0, offset + 1);
    }

    return fileNameStr;
}

std::string vku::getSpvFileName(const std::string& fileName)
{
    std::string fileNameStr(getFileNameWoExt(fileName));
    size_t offset = 0;
    while ((offset = fileNameStr.find('.', offset)) != std::string::npos) {
        fileNameStr.replace(offset, 1, 1, '-');
    }
    return fileNameStr + ".spv";
}

std::stringstream vku::glslCompile(const char* fileName, int shader_type)
{
    std::stringstream stream;
    size_t size = 0;
    std::vector<uint32_t> spvBinary = glslCompile(fileName, size, shader_type);

    if (spvBinary.size() != 0) {
        stream.rdbuf()->sputn((char*)spvBinary.data(), size);
    }

    return stream;
}

std::vector<uint32_t> vku::glslCompile(const char* fileName, size_t& size, int shader_type)
{
    std::vector<uint32_t> spvBinary;

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    std::string fName;

    if ((fName = getShaderFileName(fileName)).empty() == false) {
        std::ifstream t(fName);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

        //options.AddMacroDefinition("MY_DEFINE", "1");
        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(str, static_cast<shaderc_shader_kind>(shader_type), "shader_src", options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::string msg = module.GetErrorMessage();
        } else {
            spvBinary.assign(module.cbegin(), module.cend()); 
            size = (module.cend() - module.cbegin()) * sizeof(uint32_t);

            static int count = 0;
            if (m_enableSPVDump) {
                std::string fileName("shader");
                fileName += std::to_string(count++) + ".spv";
                FILE* fptr = fopen(fileName.c_str(), "wb");
                fwrite(spvBinary.data(), size, 1, fptr);
                fclose(fptr);
            }
        }
    }

    return spvBinary;

}

void* vku::glslRead(const char* fileName, size_t& size) 
{
    std::string glslFileName;
    std::string outSpvFileName;

    if ((glslFileName = getShaderFileName(fileName)).empty() == false) {
        outSpvFileName = getSpvFileName(glslFileName);
        if (glsl2spv(glslFileName, outSpvFileName) == 0) {
            FILE* fp = fopen(outSpvFileName.c_str(), "rb");
            if (!fp) {
                return nullptr;
            }

            fseek(fp, 0L, SEEK_END);
            size = ftell(fp);

            fseek(fp, 0L, SEEK_SET);

            void* shader_code = malloc(size);
            size_t retval = fread(shader_code, size, 1, fp);
            fclose(fp);
            
            return shader_code;
        }
    }
    return nullptr;
}

int vku::glsl2spv(const std::string& glslFileName, const std::string& outSpvFileName)
{
    std::string cmdLine = std::string("glslangValidator -s -V -o") + outSpvFileName + " " + glslFileName;
    return execCmd(cmdLine);
}

int vku::execCmd(std::string& cmd) 
{
#if defined(WIN32)
    #define popen _popen
    #define pclose _pclose
#elif defined(LINUX)
#endif

    int rtnVal = 0;
    std::array<char, 128> buffer;
    std::string result;
    //std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    //if (!pipe) {
    //    throw std::runtime_error("popen() failed!");
    //}
    //while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    //    result += buffer.data();
    //}
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
        return -1;
    }
    while (fgets(buffer.data(), 128, pipe) != NULL) {
        result += buffer.data();
    }
    return pclose(pipe);
}

