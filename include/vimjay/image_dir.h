/** Copyright 2012 Lucas Walter */
#ifndef VIMJAY_IMAGE_DIR_H
#define VIMJAY_IMAGE_DIR_H

#include <iostream>
#include <stdio.h>

#include <boost/thread.hpp>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

// #include <random>
#include <deque>
#include <map>
#include <vector>
#include <string>

#include "vimjay/nodes.h"

namespace bm
{
/////////////////////////////////
class ImageDir : public Buffer
{
  std::deque<cv::Mat> frames_orig;
  std::vector<std::string> all_files;
  bool resizeImages();

public:
  explicit ImageDir(const std::string name);
  virtual void init();

  bool loadImages();

  virtual bool update();
  virtual bool load(cv::FileNodeIterator nd);
  virtual bool save(cv::FileStorage& fs);
};
}  // namespace bm
#endif  // VIMJAY_IMAGE_DIR_H
