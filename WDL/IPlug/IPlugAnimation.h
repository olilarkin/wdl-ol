#ifndef _IPLUGANIMATION_
#define _IPLUGANIMATION_
/*
Youlean - IPlugAnimation - Adding easy animations to IPlug

Copyright (C) 2016 and later, Youlean

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. This notice may not be removed or altered from any source distribution.

*/

#include <vector>
#include <algorithm>
#include <string>
#include "IPlugBase.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846264338328      // Pi 
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923132169164      // Pi/2 
#endif

#define SMOOTHSTEP(x) ((x) * (x) * (3 - 2 * (x)))

/*
Use this site to create custom bezier animations: https://matthewlein.com/ceaser/
See easing curves here: http://easings.net/
*/

using namespace std;

typedef enum _animationFlag
{
	_LinearInterpolation,

	_QuadraticEaseIn,
	_QuadraticEaseOut,
	_QuadraticEaseInOut,

	_CubicEaseIn,
	_CubicEaseOut,
	_CubicEaseInOut,

	_QuarticEaseIn,
	_QuarticEaseOut,
	_QuarticEaseInOut,

	_QuinticEaseIn,
	_QuinticEaseOut,
	_QuinticEaseInOut,

	_SineEaseIn,
	_SineEaseOut,
	_SineEaseInOut,

	_CircularEaseIn,
	_CircularEaseOut,
	_CircularEaseInOut,

	_ExponentialEaseIn,
	_ExponentialEaseOut,
	_ExponentialEaseInOut,

	_ElasticEaseIn,
	_ElasticEaseOut,
	_ElasticEaseInOut,

	_BackEaseIn,
	_BackEaseOut,
	_BackEaseInOut,

	_BounceEaseIn,
	_BounceEaseOut,
	_BounceEaseInOut,

	_BezierEase,
	_BezierEaseIn,
	_BezierEaseOut,
	_BezierEaseInOut,
	_BezierSwiftMove,
	_BezierSwifterMove,
	_BezierHeavyMove,
	_BezierSwiftIn,
	_BezierSwiftOut,
	_BezierCustom
}
animationFlag;

class IPlugAnimation
{
public:
	IPlugAnimation() {}

	~IPlugAnimation() {}

	double Animation(const char* uniqueAnimationName, bool state, double start, double end, int startToEndFrames, int endToStartFrames, 
		animationFlag startToEnd = _LinearInterpolation, animationFlag endToStart = _LinearInterpolation);
		
	void DrawAnimationCurve_DEBUG(IGraphics* pGraphics, animationFlag flag, int size = 300, int x = 0, int y = 0);

	void SetCustomBezier(double X1, double Y1, double X2, double Y2);

	void UsingSmoothStep();

	void DisableAnimation(const char* animationName)
	{
		int position = ReturnAnimationPosition(animationName);

		if (position != -1)
			animation_is_disabled[position] = true;
	}

	void EnableAnimation(const char* animationName)
	{
		int position = ReturnAnimationPosition(animationName);

		if (position != -1)
			animation_is_disabled[position] = false;
	}
	
	bool AnimationRequestDirty();

private:

	int ReturnAnimationPosition(const char* animationName)
	{
		string uniqueName = animationName;
		// Find animation
		for (int i = 0; i < animation_unique_name.size(); i++)
			if (animation_unique_name[i] == uniqueName) return i;

		return -1;
	}

	double LinearInterpolate(double y1, double y2, double mu);

	double TransferFunction(double start, double end, double pos, animationFlag using_animation);
	
	// Bezier stuff --------------------------------------------------------------------------------------
	double BezierX(double t, double Cx, double Bx, double Ax);

	double BezierY(double t, double Cy, double By, double Ay);

	double BezierXDer(double t, double Cx, double Bx, double Ax);

	double FindXFor(double t, double Cx, double Bx, double Ax);

	double CubicBezier(double m, double p1, double p2, double p3, double p4);

	// Easing functions -----------------------------------------------------------------------------------
	double LinearInterpolation(double p);

	double QuadraticEaseIn(double p);

	double QuadraticEaseOut(double p);

	double QuadraticEaseInOut(double p);

	double CubicEaseIn(double p);

	double CubicEaseOut(double p);

	double CubicEaseInOut(double p);

	double QuarticEaseIn(double p);

	double QuarticEaseOut(double p);

	double QuarticEaseInOut(double p);

	double QuinticEaseIn(double p);

	double QuinticEaseOut(double p);

	double QuinticEaseInOut(double p);

	double SineEaseIn(double p);

	double SineEaseOut(double p);

	double SineEaseInOut(double p);

	double CircularEaseIn(double p);

	double CircularEaseOut(double p);

	double CircularEaseInOut(double p);

	double ExponentialEaseIn(double p);

	double ExponentialEaseOut(double p);

	double ExponentialEaseInOut(double p);

	double ElasticEaseIn(double p);

	double ElasticEaseOut(double p);

	double ElasticEaseInOut(double p);

	double BackEaseIn(double p);

	double BackEaseOut(double p);

	double BackEaseInOut(double p);

	double BounceEaseIn(double p);

	double BounceEaseOut(double p);

	double BounceEaseInOut(double p);

	vector <bool> animation_redraw;
	vector <bool> animation_last_state;
	vector <string> animation_unique_name;
	vector <bool> animation_state;
	vector <int> animation_frame;
	vector <double> animation_start;
	vector <double> animation_end;
	vector <double> animation_pos;
	vector <double> animation_last_pos;
	vector <int> animation_start_to_end_frames;
	vector <int> animation_end_to_start_frames;
	vector <bool> animation_is_disabled;

	double x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0;
	bool smooth_step = false;
};

#endif