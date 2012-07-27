
#include <iostream>
#include <stdio.h>

#include <boost/thread.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <glog/logging.h>

#include <deque>
//#include <pair>

#include "nodes.h"

using namespace cv;
using namespace std;

namespace bm {


  ////////////////////////////////////////////////
  Node::Node() {
    do_update = false;

    //is_dirty = true;
    vcol = cv::Scalar(0,128,255);
  }

  // this finds all the nodes that are connected to this node and sets them to be updated
  bool Node::setUpdate()
  {
    // if do_update is already set return, this prevents infinite loops
    if (do_update) return false;

    do_update = true;

    //LOG(INFO) << "in sz " << inputs.size();
    for (int i = 0; i < inputs.size(); i++) {
      inputs[i]->setUpdate();  
    }
    
    return true;
  }

  bool Node::isDirty(void* caller, bool clear) 
  {
    std::map<void*, bool>::iterator is_dirty;  
    is_dirty = dirty_hash.find(caller);

    if (is_dirty == dirty_hash.end()) {
      dirty_hash[caller] = false;
      return true;
    }

    const bool rv = is_dirty->second;
    if (clear) {
      dirty_hash[caller] = false;
    }
    return rv;
  }

  bool Node::setDirty()
  {
    for (map<void*,bool>::iterator it = dirty_hash.begin() ; it != dirty_hash.end(); it++ ) it->second = true;
  }

  // the rv is so that an inheriting function will know whether to 
  // process or not
  bool Node::update() 
  {
    if (!do_update) return false;
    do_update = false; 

    bool inputs_dirty = false;
    for (int i = 0; i < inputs.size(); i++) {
      inputs[i]->update();
      if (inputs[i]->isDirty(this)) inputs_dirty = true;
    }

    // the inheriting object needs to set is_dirty as appropriate if it
    // isn't sufficient to have inputs determine it(e.g. it is sourcing change)
    if ( inputs_dirty ) {
      setDirty();
    }
    
    VLOG(2) << name << " in sz " << inputs.size() << " inputs dirty" << inputs_dirty;

    return true;
  }

  bool Node::draw(float scale) 
  {
    int fr = 1;
    if (!isDirty(this)) fr = 5;
    cv::Scalar col = cv::Scalar(vcol/fr);

    cv::circle(graph, loc, 20, col, 4);
  
    for (int j = 0; j < inputs.size(); j++) {
      cv::Point src = inputs[j]->loc + cv::Point(20,0);
      cv::Point mid = src + (loc -src) * 0.8;
      cv::line( graph, src, mid, cv::Scalar(0, 128/fr, 0), 2, 4 );
      cv::line( graph, mid, loc + cv::Point(-20, j*5), cv::Scalar(0, 255/fr, 0), 2, CV_AA );
    }

    cv::putText(graph, name, loc - cv::Point(0,5), 1, 1, cv::Scalar(255,255,245));

  }

  //////////////////////////////////
  ImageNode::ImageNode() : Node()
  {
    vcol = cv::Scalar(255,0,255);
  }

  // TBD could there be a templated get function to be used in different node types?
  cv::Mat ImageNode::get() {
    return out;//_old;
  }
 
  /// Probably don't want to call this in most inheriting functions, skip back to Node::update()
  bool ImageNode::update() 
  {
    const bool rv = Node::update();
    if (!rv) return false;

    if (inputs.size() > 0) {
      ImageNode* im_in = dynamic_cast<ImageNode*> (inputs[0]);
      if (im_in) {
        
        //out_old = out;//.clone(); // TBD need to clone this?  It doesn't work
        cv::Mat new_out = im_in->get(); 

        out_old = out;
        if (new_out.refcount == out.refcount) {
          VLOG(2) << "dirty input is identical with old image " << new_out.refcount << " " << out.refcount;
          out = new_out.clone();
        } else {
          out = new_out;
        }

      } // im_in
    }  // inputs
    VLOG(3) << name << " update: " <<  out.refcount << " " << out_old.refcount;
  
    return true;
  }

  bool ImageNode::draw(float scale) 
  {
    Node::draw(scale);

    if (!out.empty()) {

      cv::Size sz = cv::Size(out.size().width * scale, out.size().height * scale);

      cv::Mat thumbnail = cv::Mat(sz, CV_8UC3);
      //cv::resize(tmp->get(), thumbnail, thumbnail.size(), 0, 0, cv::INTER_NEAREST );
      cv::resize(out, thumbnail, sz, 0, 0, cv::INTER_NEAREST );
      //cv::resize(tmp->get(), thumbnail, cv::INTER_NEAREST );
       
      int fr = 1;
      if (!isDirty(this)) fr = 5;
      cv::Scalar col = cv::Scalar(vcol/fr);

      cv::rectangle(graph, loc - cv::Point(2,2), loc + cv::Point(sz.width,sz.height) + cv::Point(2,2), col, CV_FILLED );
      cv::Mat graph_roi = graph(cv::Rect(loc.x, loc.y, sz.width, sz.height));
      graph_roi = cv::Scalar(0, 0, 255);
      thumbnail.copyTo(graph_roi);
    }
  }

  ////////////////////////////////////////////////////////////
  
  Rot2D::Rot2D()
  {
    angle = 0;
    scale = 1.0;
    center = cv::Point2f(0,0);
  }

  bool Rot2D::update()
  {
    if (!Node::update()) return false;

    if (inputs.size() < 1) return false;
    
    ImageNode* im_in = dynamic_cast<ImageNode*> (inputs[0]);
    if (!im_in) return false;
     
    //VLOG(1) << name << " " << is_dirty << " " << im_in->name << " " << im_in->is_dirty;

    cv::Mat rot = cv::getRotationMatrix2D(center, angle, scale);

    cv::Mat tmp;
    cv::warpAffine(im_in->get(), tmp, rot, im_in->get().size(), INTER_NEAREST);

    out = tmp;
  }

  ////////////////////////////////////////////////////////////
  Webcam::Webcam()
  {
    LOG(INFO) << "camera opening ...";
    capture = VideoCapture(0); //CV_CAP_OPENNI );
    LOG(INFO) << "done.";

    if ( !capture.isOpened() ) {
      LOG(ERROR) << "Can not open a capture object.";
      return;// -1;
    }

        //bool rv1 = capture.set( CV_CAP_PROP_FRAME_WIDTH, 800);
        //    //bool rv2 = capture.set( CV_CAP_PROP_FRAME_HEIGHT, 600);
        //        //LOG(INFO) << "set res " << rv1 << " " << rv2;
        //
        //
    

    //update();
    //update();
    is_thread_dirty = false;
    boost::thread cam_thread(&Webcam::runThread,this);

    // wait for single frame so there is a sample with the correct size
    while(!is_thread_dirty) {}
  }

  void Webcam::runThread()
  {
    do_capture = true;
    while(true) {
      if (do_capture) {
        /// TBD need to make this in separate thread
        if( !capture.grab() )
        {
          LOG(ERROR) << name << " Can not grab images." << endl;
          //return false;
        } 

        cv::Mat new_out;
        capture.retrieve(new_out);

        if (new_out.empty()) {
          LOG(ERROR) << name << " new image empty";
          //return false;
        }
        // I think opencv is reusing a mat within capture so have to clone it

        //if (&new_out.data == &out.data) {
        //
        cv::Mat tmp; // new_out.clone();
       
        float scale = 1.0;
        if (MAT_FORMAT == CV_16S) scale = 255;
        if (MAT_FORMAT == CV_32F) scale = 1.0/255.0;
        if (MAT_FORMAT == CV_8U) scale = 1.0;
        new_out.convertTo(tmp, MAT_FORMAT,scale); //, 1.0/(255.0));//*255.0*255.0*255.0));

        //out_lock.lock();
        out = tmp;
        is_thread_dirty = true;
        //out_lock.unlock();
        //} else {
        //  VLOG(3) << name << " dissimilar capture";
        //  out = new_out;
        //}

        // TBD out is the same address every time, why doesn't clone produce a new one?
        //VLOG(3) << 

      } else {
        usleep(1000);
      }
    }
  } // runThread

/*
  cv::Mat get()
  {

  }
  */

  bool Webcam::update()
  {
    ImageNode::update();

      if ( is_thread_dirty ) setDirty();
      is_thread_dirty = false;

    return true;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // TBD subclasses of Node that are input/output specific, or make that general somehow?
  Signal::Signal() : Node()
  {
    vcol = cv::Scalar(0,128,255);
  }

  void Signal::setup(const float new_step, const float offset) 
  {
    value = offset;
    step = new_step;
    LOG(INFO) << "Signal " << value << " " << new_step;
  }
  
  bool Signal::update() 
  {
    if (!Node::update()) return false;

    // it wouldn't be hard to update these
    // even if they aren't in need of updating, but don't for now
    value += step;
    if (value > 1.0) value = 0.0;
    if (value < 0.0) value = 1.0;
    //is_dirty = true;
    setDirty();

    return true;
  }

  bool Signal::draw(float scale)
  {
    Node::draw(scale);

    cv::rectangle(graph, loc, loc + cv::Point( value * 100 , 5), cv::Scalar(255, 255, 100), CV_FILLED);

    return true;
  }

  //////////////////////////////////////////////////
  Saw::Saw() : Signal()
  {
    vcol = cv::Scalar(0,90,255);
  }
  
  void Saw::setup(const float new_step, const float offset) 
  {
    Signal::setup(new_step, offset);
  }

  bool Saw::update()
  { 
    if (!Node::update()) return false;

    value += step;
    if (value > 1.0) {
      step = -abs(step);
      value = 1.0;
    }
    if (value < 0.0) {
      step = abs(step);
      value = 0.0;
    }
    setDirty();
    //is_dirty = true;

    //LOG(INFO) << step << " " << value;
    return true;
  }

  //////////////////////////////////
  Buffer::Buffer() : ImageNode() {
    //this->max_size = max_size;
    //LOG(INFO) << "new buffer max_size " << this->max_size;
    vcol = cv::Scalar(200, 30, 200);
  }
 
  bool Buffer::update()
  {
    bool rv = ImageNode::update();
    if (!rv) return false;

    for (int i = 0; i < inputs.size(); i++) {
      
      ImageNode* im_in = dynamic_cast<ImageNode*> (inputs[i]);
      if (im_in && im_in->isDirty(this)) 
        add(im_in->get()); 
    }
    
    out = frames[0];

    return true;
  }

  bool Buffer::draw(float scale) 
  {
    ImageNode::draw(scale);

    // draw some grabs of the beginning frame, and other partway through the buffer 
    for (int i = 1; i < 4; i++) {
      int ind = i * frames.size() / 3;
      if (i == 3) ind = frames.size() - 1;
      if (ind >= frames.size())  continue;

      cv::Size sz = cv::Size(out.size().width * scale * 0.25, out.size().height * scale * 0.25);

      cv::Mat thumbnail = cv::Mat(sz, CV_8UC3);
      cv::resize(frames[ind], thumbnail, sz, 0, 0, cv::INTER_NEAREST );
      //cv::resize(tmp->get(), thumbnail, cv::INTER_NEAREST );
      cv::Mat graph_roi = graph(cv::Rect(loc.x + i * out.cols*scale*0.25, loc.y + out.rows*scale, sz.width, sz.height));
      graph_roi = cv::Scalar(0, 0, 255);
      thumbnail.copyTo(graph_roi);
    }
  }

  void Buffer::add(cv::Mat new_frame)
  {
    if (new_frame.empty()) {
      LOG(ERROR) << name << " new_frame is empty";
      //is_dirty = false;
      return;// TBD LOG(ERROR)
    }
    
    if ((frames.size() > 0) && new_frame.refcount == frames[frames.size()-1].refcount) {
      new_frame = new_frame.clone();
      LOG(INFO) << name << " cloning identical frame " << new_frame.refcount << " " << frames[frames.size()-1].refcount;
    }

    frames.push_back(new_frame);

    while (frames.size() >= max_size) frames.pop_front();
   
    VLOG(3) << name << " sz " << frames.size();
    
    // TBD is_dirty wouldn't be true for callers that want frames indexed from beginning if no pop_front has been done.
    setDirty();
  }
  
  cv::Mat Buffer::get() {
    return ImageNode::get();
  }

  // not the same as the inherited get on purpose
  // many callers per time step could be calling this
  cv::Mat Buffer::get(const float fr) 
  {
    if (frames.size() < 1) {
      VLOG(1) << "no frames returning gray";
      cv::Mat tmp = cv::Mat(640, 480, CV_8UC3);
      tmp = cv::Scalar(128);
      return tmp;
    }

    int ind = (int)(fr*(float)frames.size());
    if (fr < 0) {
      ind = frames.size() - ind;
    }
    
    if (ind > frames.size() -1) ind = frames.size()-1;
    if (ind < 0) ind = 0;
    //ind %= frames.size();
   
    //VLOG_EVERY_N(1,10) 
    //LOG_EVERY_N(INFO, 10) << ind << " " << frames.size();
    return frames[ind];
  }

  // TBD get(int ind), negative ind index from last


///////////////////////////////////////////////////////////
  Tap::Tap() : ImageNode()
  {
    vcol = cv::Scalar(100, 30, 250);
  }

  void Tap::setup(Signal* new_signal, Buffer* new_buffer) 
  {
    signal = new_signal;
    buffer = new_buffer;
    
    inputs.clear();
    inputs.push_back(signal);
    inputs.push_back(buffer);
  }

  bool Tap::update()
  {
    if (!Node::update()) return false;

    if (isDirty(this)) {
      out = buffer->get(signal->value);
    }

    return true;
  }


  ///////////////////////////////////////////
  Add::Add() : ImageNode()
  {
    vcol = cv::Scalar(200, 200, 50);
  }
  
  void Add::setup(ImageNode* np1, ImageNode* np2, float nf1, float nf2) 
  {
    p1 = np1;
    p2 = np2;

    f1 = nf1;
    f2 = nf2;
   
    inputs.push_back(p1);
    inputs.push_back(p2);
  }

  bool Add::update()
  {
    if (!Node::update()) return false;

    //VLOG(1) << "name " << is_dirty << " " << p1->name << " " << p1->is_dirty << ", " << p2->name << " " << p2->is_dirty ;
    if (isDirty(this)) {
      // TBD accomodate bad mats somewhere
      out = p1->get() * f1 + p2->get() * f2;
    }

    return true;
  }


}  // namespace bm

