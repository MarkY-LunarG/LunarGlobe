#!/usr/bin/python3 -i
#
# Copyright (c) 2018, LunarG, Inc
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import sys
import getopt
import subprocess

def main(argv):
    glslang_validator = "glslangValidator"
    try:
        opts, args = getopt.getopt(argv,"hg:",["glslang_dir="])
    except getopt.GetoptError:
        print ('generate_spirv.py -g <glslang_directory>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print ('generate_spirv.py -g <glslang_directory>')
            sys.exit()
        elif opt in ("-g", "--glslang_dir"):
            glslang_folder = os.path.join(arg, 'bin')
            validator = os.path.join(glslang_folder, glslang_validator)
            glslang_validator = validator

    current_path = os.getcwd()
    shader_src_subfolder = 'shaders/source'
    shader_dst_subfolder = 'shaders'
    shader_src_full_path = os.path.join(current_path, shader_src_subfolder)
    shader_dst_full_path = os.path.join(current_path, shader_dst_subfolder)
    input_dir = os.fsencode(shader_src_full_path)
    for file in os.listdir(input_dir):
        filename = os.fsdecode(file)
        if filename.endswith("_glsl.vert"):
            output_name = filename.replace('_glsl.vert', '-vs.spv')
            output_file = os.path.join(shader_dst_full_path, output_name)
        elif filename.endswith("_glsl.frag"): 
            output_name = filename.replace('_glsl.frag', '-fs.spv')
            output_file = os.path.join(shader_dst_full_path, output_name)
        else:
            continue
        input_file = os.path.join(shader_src_full_path, filename)
        glslang_command = '%s -g -V -o %s %s' % (glslang_validator, output_file, input_file)
        print('GLSLANG COMMAND => %s' % glslang_command)
        os.system(glslang_command)
    

if __name__ == "__main__":
   main(sys.argv[1:])


