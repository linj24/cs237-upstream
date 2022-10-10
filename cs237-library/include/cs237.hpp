/*! \file cs237.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * This is the main header file for the CS237 Library.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_HPP_
#define _CS237_HPP_

#include "cs237-config.h"

#include <cmath>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

/* The GLFW and Vulkan library */
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/* GLM include files */
#include <glm/glm.hpp>

namespace cs237 {

//! function for reporting errors by raising a runtime exception
//! that includes the file and line number of the error.
inline void ReportError (const char *file, int line, std::string const &msg)
{
    std::string s = "[" + std::string(file) + ":" + std::to_string(line) + "] " + msg;
    throw std::runtime_error(s);
}

} // namespace cs237

#define ERROR(msg)      cs237::ReportError (__FILE__, __LINE__, msg);

/* CS23700 support files */
#include "cs237-types.hpp"

#include "json.hpp"
#include "cs237-shader.hpp"
#include "cs237-pipeline.hpp"
#include "cs237-application.hpp"
#include "cs237-window.hpp"
#include "cs237-memory-obj.hpp"
#include "cs237-buffer.hpp"

#endif // !_CS237_HPP_
