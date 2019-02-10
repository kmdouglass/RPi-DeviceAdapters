/**
 * Kyle M. Douglass, 2019
 * kyle.m.douglass@gmail.com
 *
 * Utilities containing information specific to a given image format.
 *
 */

#ifndef _RPiV4L2_FORMATS_H_
#define _RPiV4L2_FORMATS_H_

#include <map>
#include <string>


// https://stackoverflow.com/questions/2636303/how-to-initialize-a-private-static-const-map-in-c
struct FourCC {
  /**
   * The number of components per pixel for a four character code.
   */
  static std::map<std::string, unsigned int> num_components() {
    std::map<std::string, unsigned int> m;
    m["RGB1"] = 3;
    m["RGB3"] = 3;
    m["RGB4"] = 4;
    m["YUYV"] = 2;
    m["YVYU"] = 2;
    m["VYUY"] = 2;
    m["UYVY"] = 2;
    m["BGR3"] = 3;
    m["BGR4"] = 4;
    return m;
  }

  static const std::map<std::string, unsigned int> COMPONENTS;
};

const std::map< std::string, unsigned int > FourCC::COMPONENTS = FourCC::num_components();

#endif // _RPiV4L2_FORMATS_H_
