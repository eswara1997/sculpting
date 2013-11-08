#include "StdAfx.h"
#include "LeapInteraction.h"
#include "cinder/app/AppBasic.h"
#include "Utilities.h"
#include <vector>

using namespace ci;

#define USE_SKELETON_API 0

const float LeapInteraction::MIN_POINTABLE_LENGTH = 10.0f;
const float LeapInteraction::MIN_POINTABLE_AGE = 0.05f;

LeapInteraction::LeapInteraction(Sculpt* sculpt, UserInterface* ui)
  : _sculpt(sculpt)
  , _ui(ui)
  , _desired_brush_radius(0.4f)
  , _is_pinched(false)
  , _last_camera_update_time(0.0)
  , _autoBrush(true)
{
  _dphi.value = 0.0f;
  _dtheta.value = 0.0f;
  _dzoom.value = 0.0f;
}

bool LeapInteraction::processInteraction(LeapListener& listener, float aspect, const Matrix44f& modelView, const Matrix44f& projection, const Vec2i& viewport, float referenceDistance, float fov, bool suppress)
{
  static const double MIN_TIME_BETWEEN_FRAMES = 0.000001;
  _model_view_inv = modelView.inverted();
  _model_view = modelView;
  _projection = projection;
  _window_size = viewport;
  _reference_distance = referenceDistance;
  _fov = fov;
  if (suppress)
  {
    _cur_frame = Leap::Frame::invalid();
  }
  else if (listener.isConnected() && listener.waitForFrame(_cur_frame, 33))
  {
    std::unique_lock<std::mutex> brushLock(_sculpt->getBrushMutex());
    std::unique_lock<std::mutex> tipsLock(_tips_mutex);
    const double time = Utilities::TIME_STAMP_TICKS_TO_SECS*static_cast<double>(_cur_frame.timestamp());
    const double prevTime = Utilities::TIME_STAMP_TICKS_TO_SECS*static_cast<double>(_last_frame.timestamp());
    if (_last_frame.isValid() && fabs(time - prevTime) > MIN_TIME_BETWEEN_FRAMES) {
      _sculpt->clearBrushes();
      _tips.clear();
      updateHandInfos(time);
      interact(time);
      cleanUpHandInfos(time);
    }
    _last_frame = _cur_frame;
    return true;
  }
  return false;
}

void LeapInteraction::interact(double curTime)
{
  float cur_dtheta = 0;
  float cur_dphi = 0;
  float cur_dzoom = 0;
  static const float ORBIT_SPEED = 0.008f;
  static const float ZOOM_SPEED = 100.0f;
  static const float AGE_WARMUP_TIME = 0.5f;
  static const float TARGET_DELTA_TIME = 1.0f / 60.0f;

  // create brushes
  static const Vec3f LEAP_OFFSET(0, 200, 50);
  static const Vec3f LEAP_SIZE(300, 300, 300);
  Leap::HandList hands = _cur_frame.hands();
  const float ui_mult = 1.0f - _ui->maxActivation();
  //const int num_hands = hands.count();
  const float deltaTime = static_cast<float>(Utilities::TIME_STAMP_TICKS_TO_SECS*(_cur_frame.timestamp() - _last_frame.timestamp()));
  const float dtMult = deltaTime / TARGET_DELTA_TIME;
  const Vector3 scaledSize = calcSize(_fov, _reference_distance);

  for (HandInfoMap::iterator it = _hand_infos.begin(); it != _hand_infos.end(); ++it) {
    const int id = it->first;
    const HandInfo& cur = it->second;
    const int numFingers = cur.getNumFingers();
    const float normalY = cur.getNormalY();
    if (cur.getLastUpdateTime() < curTime) {
      continue;
    }
    if (numFingers > 2 || normalY < 0.35f) {
      // camera interaction
      const Vector3 movement = cur.getModifiedTranslation();
      cur_dtheta += ORBIT_SPEED * movement.x();
      cur_dphi += ORBIT_SPEED * -movement.y();
      cur_dzoom += ZOOM_SPEED * -movement.z();
      _last_camera_update_time = ci::app::getElapsedSeconds();
    } else {
      // sculpting interaction
      const Leap::Hand hand = _cur_frame.hand(id);
      if (hand.isValid()) {
        const Leap::PointableList pointables = hand.pointables();
        const int num_pointables = pointables.count();
        for (int j=0; j<num_pointables; j++) {
          if (pointables[j].length() < MIN_POINTABLE_LENGTH || pointables[j].timeVisible() < MIN_POINTABLE_AGE) {
            continue;
          }

          // add brushes
          const float strengthMult = Utilities::SmootherStep(math<float>::clamp(pointables[j].timeVisible()/AGE_WARMUP_TIME));

          Leap::Vector tip_pos = pointables[j].tipPosition();
          Leap::Vector tip_dir = pointables[j].direction();
          Leap::Vector tip_vel = pointables[j].tipVelocity();

          Vec3f pos = Vec3f(tip_pos.x, tip_pos.y, tip_pos.z) - LEAP_OFFSET;
          Vec3f dir = Vec3f(tip_dir.x, tip_dir.y, tip_dir.z);
          Vec3f vel = Vec3f(tip_vel.x, tip_vel.y, tip_vel.z);

          float ratioX = pos.x / LEAP_SIZE.x;
          float ratioY = pos.y / LEAP_SIZE.y;
          float ratioZ = pos.z / LEAP_SIZE.z;

          pos.x = ratioX * scaledSize.x();
          pos.y = ratioY * scaledSize.y();
          pos.z = ratioZ * scaledSize.z();

          Vector3 brushPos(_model_view_inv.transformPoint(pos).ptr());
          Vector3 brushDir((-_model_view_inv.transformVec(dir)).ptr());
          Vector3 brushVel(_model_view_inv.transformVec(vel).ptr());

          float strength = strengthMult*ui_mult*_desired_brush_strength;
          strength = std::min(1.0f, strength * dtMult);

          Vec3f transPos = _projection.transformPoint(pos);
          Vec3f radPos = _projection.transformPoint(pos+Vec3f(_desired_brush_radius, 0, 0));

          // compute screen-space coordinate of this finger
          transPos.x = (transPos.x + 1)/2;
          transPos.y = (transPos.y + 1)/2;
          transPos.z = 1.0f;

          const float autoBrushScaleFactor = (scaledSize.x() / LEAP_SIZE.x);
          if (transPos.x >= -0.1f && transPos.x <= 1.1f && transPos.y >= -0.1f && transPos.y <= 1.1f) {
            float adjRadius = _desired_brush_radius;
            if (_autoBrush) {
              adjRadius *= autoBrushScaleFactor;
            }
            if (strengthMult > 0.25f) {
              _sculpt->addBrush(Vector3(pos.ptr()), brushPos, brushDir, brushVel, adjRadius, strength, strengthMult);
            }
          }

          // compute a point on the surface of the sphere to use as the screen-space radius
          radPos.x = (radPos.x + 1)/2;
          radPos.y = (radPos.y + 1)/2;
          radPos.z = 1.0f;
          float rad = transPos.distance(radPos);
          if (_autoBrush) {
            rad *= autoBrushScaleFactor;
          }
          transPos.z = rad;
          Vec4f tip(transPos.x, transPos.y, transPos.z, strengthMult);
          _tips.push_back(tip);
        }
      }
    }
  }

  cur_dtheta /= deltaTime;
  cur_dphi /= deltaTime;
  cur_dzoom /= deltaTime;

  static const float SMOOTH_STRENGTH = 0.9f;
  _dtheta.Update(cur_dtheta, curTime, SMOOTH_STRENGTH);
  _dphi.Update(cur_dphi, curTime, SMOOTH_STRENGTH);
  _dzoom.Update(cur_dzoom, curTime, SMOOTH_STRENGTH);

#if USE_SKELETON_API
  //// Handle pinching
  //const int numHands = _cur_frame.hands().count();
  //for (int ih = 0; ih < numHands; ih++) {
  //  Leap::Hand& hand = _cur_frame.hands()[ih];
  //  LM_LOG << "Manipulation strength " << float(hand.manipulationStrength()) << std::endl;
  //}

  static const float PINCH_START_THRESHOLD = 0.8f;
  static const float PINCH_END_THRESHOLD = 0.7f;
  // Handle pinching
  if (_is_pinched) {
    Leap::Hand& hand = _cur_frame.hand(_pinching_hand_id);
    // Handle pinch end
    if (!hand.isValid() || hand.manipulationStrength() < PINCH_END_THRESHOLD) {
      _is_pinched = false;
    } else {
      // Handle pinch drag
      _pinch_last_recorded = ToVec3f(hand.manipulationPoint().toVector3<Vector3>());

      // Check for pinning a movement direction
      if (!_pin_z && !_pin_xy) {
        Vec3f diff = _pinch_last_recorded - _pinch_origin;
        if (20.0f < diff.length()) {
          // Check angle and decide on direction
          if (std::sqrt(diff[0]*diff[0]+diff[1]*diff[1]) < std::fabs(diff[2]) ) {
            // use z
            _pin_xy = true;
          } else {
            // use xy
            _pin_z = true;
          }
        }
      }
    }
  } else {
    // Handle pinch start
    const int numHands = _cur_frame.hands().count();
    for (int ih = 0; ih < numHands; ih++) {
      Leap::Hand& hand = _cur_frame.hands()[ih];
      if (PINCH_START_THRESHOLD < hand.manipulationStrength()) {
        _is_pinched = true;
        _pin_z = false;
        _pin_xy = false;
        _pinching_hand_id = hand.id();
        _pinch_origin = ToVec3f(hand.manipulationPoint().toVector3<Vector3>());
        _pinch_last_read = _pinch_origin;
        _pinch_last_recorded = _pinch_origin;
      }
      //LM_LOG << "Manipulation strength " << float(hand.manipulationStrength()) << std::endl;
    }
  }
#endif
}

Vec3f LeapInteraction::getPinchDeltaFromLastCall() {
  if (!_is_pinched) {
    return Vec3f(0.0f, 0.0f, 0.0f);
  } else {
    Vec3f result = _pinch_last_recorded - _pinch_last_read;
    _pinch_last_read = _pinch_last_recorded;
    if (_pin_xy) {
      result[0] = 0.0f;
      result[1] = 0.0f;
    } 
    if (_pin_z) {
      result[2] = 0.0f;
    }
    return result;
  }
}

void LeapInteraction::updateHandInfos(double curTime) {
  const Leap::HandList hands = _cur_frame.hands();
  for (int i=0; i<hands.count(); i++) {
    int id = hands[i].id();
    _hand_infos[id].update(hands[i], _last_frame, curTime);
  }
}

void LeapInteraction::cleanUpHandInfos(double curTime) {
  static const float MAX_HAND_INFO_AGE = 0.1f; // seconds since last update until hand info gets cleaned up
  HandInfoMap::iterator it = _hand_infos.begin();
  while (it != _hand_infos.end()) {
    HandInfo& cur = it->second;
    const float curAge = fabs(static_cast<float>(curTime - cur.getLastUpdateTime()));
    if (curAge > MAX_HAND_INFO_AGE) {
      _hand_infos.erase(it++);
    } else {
      ++it;
    }
  }
}
