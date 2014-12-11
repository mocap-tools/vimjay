#ifndef __MISC_NODES_H__
#define __MISC_NODES_H__

#include <iostream>
#include <stdio.h>

#include <boost/thread.hpp>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//#include <random>
#include <deque>
#include <map>

#include "nodes.h"

namespace bm {


/////////////////////////////////
class ImageDir : public Buffer
{
  std::deque<cv::Mat> frames_orig;
  std::vector<std::string> all_files;
  bool resizeImages();

  public:

  ImageDir(const std::string name);
  virtual void init();

  bool loadImages();

  virtual bool update();
  virtual bool load(cv::FileNodeIterator nd);
  virtual bool save(cv::FileStorage& fs);
};

/////////////////////////////////
class BrowseDir : public ImageNode
{
  public:

  BrowseDir(const std::string name);
  virtual void init();
  
  virtual bool handleKey(int key);
  virtual bool update();
  
  private:
  boost::thread browse_thread;
  void runThread();
  
  boost::mutex dirs_mutex;
  std::vector<std::string> file_names;
  std::vector<std::string> sub_dirs;
  std::vector<int> num_sub_images;
  std::vector<int> num_sub_dirs;

};




} // bm
#endif // MISC_NODES