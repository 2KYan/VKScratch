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

#ifndef _VKU_H
#define _VKU_H
#pragma once


class vku {
    enum class ResourceType {
        SHADER = 0,
        TEXTURE,
        NUM_RESOURCES,
    };
public:
    virtual ~vku();
    static vku* instance();

    std::string getShaderFileName(const char* fileName);
    std::string getTextureFileName(const char* fileName);
    int execCmd(std::string & cmd);
    void* glslRead(const char* fileName, size_t& size);
    std::vector<uint32_t> glslCompile(const char* fileName, size_t& size, int shader_type);
    std::stringstream glslCompile(const char* fileName, int shader_type);

    int numResPaths();
protected:
    int initResPaths();
    std::string getResourceFileName(const std::string& fileName, ResourceType resType);
    std::string getFileNameWoExt(const std::string& fileName);
    std::string getSpvFileName(const std::string& fileName);
    int glsl2spv(const std::string& glslFileName, const std::string& spvFileName);

private:
    vku();
    vku(vku const&) = delete;
    vku& operator=(vku const &) = delete;
    vku(vku &&) = delete;
    vku& operator=(vku &&) = delete;
    
private:
    bool m_enableSPVDump;
    std::vector<std::string> resPaths[static_cast<int>(ResourceType::NUM_RESOURCES)];
};

#endif//_VKU_H
