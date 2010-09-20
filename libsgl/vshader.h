#ifndef _vshaderVshader_h_
#define _vshaderVshader_h_


// Header generated from binary by WriteAsBinHeader()..
static const int vshaderVshaderLength = 58;
static const unsigned int vshaderVshader[vshaderVshaderLength]={
	0x20205356,	0xFFFF0008,	0x00000048,	0x01020000,	0x0000000A,	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x00000000,
	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x00000000,	0x02000000,
	0x237820E4,	0x00000000,	0x00E40100,	0x02015500,	0x2EF820E4,	0x00000000,	0x00E40100,	0x0202AA00,	0x2EF820E4,	0x00000000,
	0x00E40100,	0x0203FF00,	0x0EF800E4,	0x00000000,	0x00000000,	0x00010000,	0x00F801E4,	0x00000000,	0x00000000,	0x00020000,
	0x00F802E4,	0x00000000,	0x00000000,	0x00030000,	0x00F803E4,	0x00000000,	0x00000000,	0x00040000,	0x00F804E4,	0x00000000,
	0x00000000,	0x00050000,	0x00F805E4,	0x00000000,	0x00000000,	0x00000000,	0x1E000000,	0x00000000,};

//checksum generated by simpleCheckSum()
static const unsigned int vshaderVshaderCheckSum = 38;

static const char* vshaderVshaderText = 
	"# Vertex shader code for fixed pipeline emulation\n"
	"# S3C6410 FIMG-3DSE v.1.5\n"
	"#\n"
	"# Copyright 2010 Tomasz Figa <tomasz.figa@gmail.com>\n"
	"#\n"
	"# This program is free software: you can redistribute it and/or modify\n"
	"# it under the terms of the GNU Lesser General Public License as published by\n"
	"# the Free Software Foundation, either version 3 of the License, or\n"
	"# (at your option) any later version.\n"
	"#\n"
	"# This program is distributed in the hope that it will be useful,\n"
	"# but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"# GNU Lesser General Public License for more details.\n"
	"#\n"
	"# You should have received a copy of the GNU Lesser General Public License\n"
	"# along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
	"#\n"
	"\n"
	"# Vertex shader version\n"
	"vs_3_0\n"
	"\n"
	"# FIMG version >= 1.2\n"
	"fimg_version    0x01020000\n"
	"\n"
	"# Shader code\n"
	"label start\n"
	"    # Transform position by transformation matrix\n"
	"    mul r0.xyzw, c0.xyzw, v0.xxxx\n"
	"    mad r0.xyzw, c1.xyzw, v0.yyyy, r0.xyzw\n"
	"    mad r0.xyzw, c2.xyzw, v0.zzzz, r0.xyzw\n"
	"    mad o0.xyzw, c3.xyzw, v0.wwww, r0.xyzw\n"
	"\n"
	"    # Pass vertex normal\n"
	"    mov o1, v1\n"
	"\n"
	"    # Pass vertex color\n"
	"    mov o2, v2\n"
	"\n"
	"    # Pass vertex point size\n"
	"    mov o3, v3\n"
	"\n"
	"    # Pass vertex texcoord 0\n"
	"    mov o4, v4\n"
	"\n"
	"    # Pass vertex texcoord 1\n"
	"    mov o5, v5\n"
	"\n"
	"    # Return\n"
	"    ret\n"
	"# End of shader code\n"
	"";

#ifdef GL_HELPERS_INCLUDED
//glHelpers.h must be included BEFORE any of the shader header files. Also make sure you have the latest version of glHelpers.h
static ghShader vshaderVshaderShader(vshaderVshaderText, vshaderVshader, vshaderVshaderLength, vshaderVshaderCheckSum);


#endif


#endif //_vshaderVshader_h_
