#include "IPlugAnimation.h"
#include "IGraphics.h"

double IPlugAnimation::Animation(const char * uniqueAnimationName, bool state, double start, double end, int startToEndFrames, int endToStartFrames, animationFlag startToEnd, animationFlag endToStart)
{
	int position = ReturnAnimationPosition(uniqueAnimationName);

	// If not found, create new animation
	if (position == -1)
	{
		animation_unique_name.push_back(uniqueAnimationName);
		animation_frame.push_back(0);
		animation_start.push_back(start);
		animation_end.push_back(end);
		animation_pos.push_back(0);
		animation_last_pos.push_back(0);
		animation_start_to_end_frames.push_back(startToEndFrames);
		animation_end_to_start_frames.push_back(endToStartFrames);
		animation_redraw.push_back(false);
		animation_last_state.push_back(state);
		animation_is_disabled.push_back(false);

		position = animation_unique_name.size() - 1;
	}

	// Disables animations
	if (animation_is_disabled[position])
	{
		if (state) animation_frame[position] = startToEndFrames;
		else animation_frame[position] = 0;
	}

	// Set frame duration for startToEnd and endToStart
	if (animation_last_state[position] != state)
	{
		animation_frame[position];
		if (state && animation_frame[position] <= 0)
		{
			animation_pos[position] = start;
			animation_start_to_end_frames[position] = startToEndFrames;
			animation_frame[position] = 0;
		}
		else if (state)
		{
			animation_pos[position] = animation_last_pos[position];
			animation_frame[position] = int((double)animation_frame[position] * ((double)startToEndFrames / (double)endToStartFrames));
			animation_start_to_end_frames[position] = startToEndFrames - animation_frame[position];
			animation_frame[position] = 0;
		}

		if (!state && animation_frame[position] >= animation_start_to_end_frames[position])
		{
			animation_pos[position] = end;
			animation_end_to_start_frames[position] = endToStartFrames;
			animation_frame[position] = endToStartFrames;
		}
		else if (!state)
		{
			animation_pos[position] = animation_last_pos[position];
			animation_frame[position] = int((double)animation_frame[position] * ((double)endToStartFrames / (double)startToEndFrames));
			animation_end_to_start_frames[position] = animation_frame[position];
		}
	}
	animation_last_state[position] = state;

	// Check if animation_frame goes out of boundries
	if (state)
	{
		if (animation_frame[position] > animation_start_to_end_frames[position]) animation_frame[position] = animation_start_to_end_frames[position];
		if (animation_frame[position] < 0) animation_frame[position] = 0;
	}
	else
	{
		if (animation_frame[position] > animation_end_to_start_frames[position]) animation_frame[position] = animation_end_to_start_frames[position];
		if (animation_frame[position] < 0) animation_frame[position] = 0;
	}

	if (state) // If it goes from start to end
	{
		if (animation_frame[position] == animation_start_to_end_frames[position])
		{
			animation_redraw[position] = false;
			return end;
		}
		else if (animation_frame[position] < animation_start_to_end_frames[position])
		{
			double mu = (double)animation_frame[position] / (double)animation_start_to_end_frames[position];
			animation_frame[position]++;
			animation_redraw[position] = true;

			return animation_last_pos[position] = TransferFunction(animation_pos[position], end, mu, startToEnd);
		}
	}
	else // If it goes from end to start
	{
		if (animation_frame[position] == 0)
		{
			animation_redraw[position] = false;
			return start;
		}
		else if (animation_frame[position] > 0)
		{
			double mu = (double)animation_frame[position] / (double)animation_end_to_start_frames[position];
			animation_frame[position]--;
			animation_redraw[position] = true;

			return animation_last_pos[position] = TransferFunction(animation_pos[position], start, 1.0 - mu, endToStart);
		}

	}
	return 0.0;
}


void IPlugAnimation::DrawAnimationCurve_DEBUG(IGraphics * pGraphics, animationFlag flag, int size, int x, int y)
{
	// Draw background
	IRECT background = IRECT(x, y, x + size, y + size);
	pGraphics->FillIRect(&COLOR_BLACK, &background);

	// Draw lines
	float lineL = float(x + size / 4);
	float lineT = float(y + size / 4);
	float lineR = float(x + size - size / 4);
	float lineB = float(y + size - size / 4);

	// X line
	pGraphics->DrawLine(&COLOR_WHITE, lineL, lineB, lineR, lineB);
	// Y line
	pGraphics->DrawLine(&COLOR_WHITE, lineL, lineB, lineL, lineT);

	// Draw text
	WDL_String text;
	IRECT rect;

	// Draw what animation is selected
	if (flag == _LinearInterpolation) text.Set("LinearInterpolation");

	if (flag == _QuadraticEaseIn) text.Set("QuadraticEaseIn");
	if (flag == _QuadraticEaseOut) text.Set("QuadraticEaseOut");
	if (flag == _QuadraticEaseInOut) text.Set("QuadraticEaseInOut");

	if (flag == _CubicEaseIn) text.Set("CubicEaseIn");
	if (flag == _CubicEaseOut) text.Set("CubicEaseOut");
	if (flag == _CubicEaseInOut) text.Set("CubicEaseInOut");

	if (flag == _QuarticEaseIn) text.Set("QuarticEaseIn");
	if (flag == _QuarticEaseOut) text.Set("QuarticEaseOut");
	if (flag == _QuarticEaseInOut) text.Set("QuarticEaseInOut");

	if (flag == _QuinticEaseIn) text.Set("QuinticEaseIn");
	if (flag == _QuinticEaseOut) text.Set("QuinticEaseOut");
	if (flag == _QuinticEaseInOut) text.Set("QuinticEaseInOut");

	if (flag == _SineEaseIn) text.Set("SineEaseIn");
	if (flag == _SineEaseOut) text.Set("SineEaseOut");
	if (flag == _SineEaseInOut) text.Set("SineEaseInOut");

	if (flag == _CircularEaseIn) text.Set("CircularEaseIn");
	if (flag == _CircularEaseOut) text.Set("CircularEaseOut");
	if (flag == _CircularEaseInOut) text.Set("CircularEaseInOut");

	if (flag == _ExponentialEaseIn) text.Set("ExponentialEaseIn");
	if (flag == _ExponentialEaseOut) text.Set("ExponentialEaseOut");
	if (flag == _ExponentialEaseInOut) text.Set("ExponentialEaseInOut");

	if (flag == _ElasticEaseIn) text.Set("ElasticEaseIn");
	if (flag == _ElasticEaseOut) text.Set("ElasticEaseOut");
	if (flag == _ElasticEaseInOut) text.Set("ElasticEaseInOut");

	if (flag == _BackEaseIn) text.Set("BackEaseIn");
	if (flag == _BackEaseOut) text.Set("BackEaseOut");
	if (flag == _BackEaseInOut) text.Set("BackEaseInOut");

	if (flag == _BounceEaseIn) text.Set("BounceEaseIn");
	if (flag == _BounceEaseOut) text.Set("BounceEaseOut");
	if (flag == _BounceEaseInOut) text.Set("BounceEaseInOut");

	if (flag == _BezierEase) text.Set("BezierEase");
	if (flag == _BezierEaseIn) text.Set("BezierEaseIn");
	if (flag == _BezierEaseOut) text.Set("ezierEaseOut");
	if (flag == _BezierEaseInOut) text.Set("BezierEaseInOut");
	if (flag == _BezierSwiftMove) text.Set("BezierSwiftMove");
	if (flag == _BezierSwifterMove) text.Set("BezierSwifterMove");
	if (flag == _BezierHeavyMove) text.Set("BezierHeavyMove");
	if (flag == _BezierSwiftIn) text.Set("BezierSwiftIn");
	if (flag == _BezierSwiftOut) text.Set("BezierSwiftOut");
	if (flag == _BezierCustom) text.Set("BezierCustom");

	int textSize = size / 20;
	IText txtIText(textSize, &COLOR_WHITE);
	txtIText.mAlign = IText::kAlignNear;

	rect = IRECT(x, y, x + size, y + size);
	pGraphics->DrawIText(&txtIText, text.Get(), &rect);

	// Draw numbers
	// XY0
	text.Set("XY0");
	rect = IRECT((int)lineL - textSize, (int)lineB, (int)lineR, (int)lineB);
	pGraphics->DrawIText(&txtIText, text.Get(), &rect);

	// X1
	text.Set("X1");
	rect = IRECT((int)lineR, (int)lineB, (int)lineR + textSize, (int)lineB);
	pGraphics->DrawIText(&txtIText, text.Get(), &rect);

	// Y1
	text.Set("Y1");
	rect = IRECT((int)lineL - textSize, (int)lineT, (int)lineR, (int)lineB);
	pGraphics->DrawIText(&txtIText, text.Get(), &rect);

	// Draw curve
	for (int i = (int)lineL; i < (int)lineR; i++)
	{
		double pos = (double)(i - (int)lineL) / (double)((int)lineR - (int)lineL);

		int pixelY = int(TransferFunction(lineB, lineT, pos, flag));

		if (i >= 0 && i <= pGraphics->Width() && pixelY >= 0 && pixelY <= pGraphics->Height())
			pGraphics->ForcePixel(&COLOR_RED, i, pixelY);
	}

	pGraphics->SetAllControlsDirty();
}

void IPlugAnimation::SetCustomBezier(double X1, double Y1, double X2, double Y2)
{
	x1 = X1;
	y1 = Y1;
	x2 = X2;
	y2 = Y2;
}

void IPlugAnimation::UsingSmoothStep() { smooth_step = true; }

bool IPlugAnimation::AnimationRequestDirty()
{
	for (int i = 0; i < animation_redraw.size(); i++)
	{
		if (animation_redraw[i] == true)
		{
			return true;
		}
	}

	return false;
}

inline double IPlugAnimation::LinearInterpolate(double y1, double y2, double mu)
{
	smooth_step = false;
	return(y1*(1 - mu) + y2*mu);
}

inline double IPlugAnimation::TransferFunction(double start, double end, double pos, animationFlag using_animation)
{
	if (smooth_step) pos = SMOOTHSTEP(pos);

	switch (using_animation)
	{
	case _LinearInterpolation: return LinearInterpolate(start, end, pos); break;

	case _QuadraticEaseIn: return LinearInterpolate(start, end, QuadraticEaseIn(pos)); break;
	case _QuadraticEaseOut: return LinearInterpolate(start, end, QuadraticEaseOut(pos)); break;
	case _QuadraticEaseInOut: return LinearInterpolate(start, end, QuadraticEaseInOut(pos)); break;

	case _CubicEaseIn: return LinearInterpolate(start, end, CubicEaseIn(pos)); break;
	case _CubicEaseOut: return LinearInterpolate(start, end, CubicEaseOut(pos)); break;
	case _CubicEaseInOut: return LinearInterpolate(start, end, CubicEaseInOut(pos)); break;

	case _QuarticEaseIn: return LinearInterpolate(start, end, QuarticEaseIn(pos)); break;
	case _QuarticEaseOut: return LinearInterpolate(start, end, QuarticEaseOut(pos)); break;
	case _QuarticEaseInOut: return LinearInterpolate(start, end, QuarticEaseInOut(pos)); break;

	case _QuinticEaseIn: return LinearInterpolate(start, end, QuinticEaseIn(pos)); break;
	case _QuinticEaseOut: return LinearInterpolate(start, end, QuinticEaseOut(pos)); break;
	case _QuinticEaseInOut: return LinearInterpolate(start, end, QuinticEaseInOut(pos)); break;

	case _SineEaseIn: return LinearInterpolate(start, end, SineEaseIn(pos)); break;
	case _SineEaseOut: return LinearInterpolate(start, end, SineEaseOut(pos)); break;
	case _SineEaseInOut: return LinearInterpolate(start, end, SineEaseInOut(pos)); break;

	case _CircularEaseIn: return LinearInterpolate(start, end, CircularEaseIn(pos)); break;
	case _CircularEaseOut: return LinearInterpolate(start, end, CircularEaseOut(pos)); break;
	case _CircularEaseInOut: return LinearInterpolate(start, end, CircularEaseInOut(pos)); break;

	case _ExponentialEaseIn: return LinearInterpolate(start, end, ExponentialEaseIn(pos)); break;
	case _ExponentialEaseOut: return LinearInterpolate(start, end, ExponentialEaseOut(pos)); break;
	case _ExponentialEaseInOut: return LinearInterpolate(start, end, ExponentialEaseInOut(pos)); break;

	case _ElasticEaseIn: return LinearInterpolate(start, end, ElasticEaseIn(pos)); break;
	case _ElasticEaseOut: return LinearInterpolate(start, end, ElasticEaseOut(pos)); break;
	case _ElasticEaseInOut: return LinearInterpolate(start, end, ElasticEaseInOut(pos)); break;

	case _BackEaseIn: return LinearInterpolate(start, end, BackEaseIn(pos)); break;
	case _BackEaseOut: return LinearInterpolate(start, end, BackEaseOut(pos)); break;
	case _BackEaseInOut: return LinearInterpolate(start, end, BackEaseInOut(pos)); break;

	case _BounceEaseIn: return LinearInterpolate(start, end, BounceEaseIn(pos)); break;
	case _BounceEaseOut: return LinearInterpolate(start, end, BounceEaseOut(pos)); break;
	case _BounceEaseInOut: return LinearInterpolate(start, end, BounceEaseInOut(pos)); break;

	case _BezierEase: return LinearInterpolate(start, end, CubicBezier(pos, 0.25, 0.1, 0.25, 1.0)); break;
	case _BezierEaseIn: return LinearInterpolate(start, end, CubicBezier(pos, 0.42, 0, 1.0, 1.0)); break;
	case _BezierEaseOut: return LinearInterpolate(start, end, CubicBezier(pos, 0, 0, 0.58, 1.0)); break;
	case _BezierEaseInOut: return LinearInterpolate(start, end, CubicBezier(pos, 0.42, 0, 0.58, 1.0)); break;
	case _BezierSwiftMove: return LinearInterpolate(start, end, CubicBezier(pos, 0.4, 0, 0.2, 1.0)); break;
	case _BezierSwifterMove: return LinearInterpolate(start, end, CubicBezier(pos, 0.4, 0, 0, 1.0)); break;
	case _BezierHeavyMove: return LinearInterpolate(start, end, CubicBezier(pos, 0.7, 0, 0.6, 1.0)); break;
	case _BezierSwiftIn: return LinearInterpolate(start, end, CubicBezier(pos, 0, 0, 0.2, 1.0)); break;
	case _BezierSwiftOut: return LinearInterpolate(start, end, CubicBezier(pos, 0.4, 0, 1.0, 1.0)); break;
	case _BezierCustom: return LinearInterpolate(start, end, CubicBezier(pos, x1, y1, x2, y2)); break;

	default: break;
	}
	return 0.0;
}

inline double IPlugAnimation::BezierX(double t, double Cx, double Bx, double Ax) { return t * (Cx + t * (Bx + t * Ax)); }

inline double IPlugAnimation::BezierY(double t, double Cy, double By, double Ay) { return t * (Cy + t * (By + t * Ay)); }

inline double IPlugAnimation::BezierXDer(double t, double Cx, double Bx, double Ax) { return Cx + t * (2 * Bx + 3 * Ax * t); }

inline double IPlugAnimation::FindXFor(double t, double Cx, double Bx, double Ax)
{
	double x = t, i = 0, z;

	while (i < 10) { // making 10 iterations max
		z = BezierX(x, Cx, Bx, Ax) - t;

		if (abs(z) < 0.001) break; // if already got close enough

		x = x - z / BezierXDer(x, Cx, Bx, Ax);
		i++;
	}

	return x;
}

inline double IPlugAnimation::CubicBezier(double m, double p1, double p2, double p3, double p4)
{
	// defining the bezier doubles in the polynomial form
	double Cx = 3 * p1;
	double Bx = 3 * (p3 - p1) - Cx;
	double Ax = 1 - Cx - Bx;

	double Cy = 3 * p2;
	double By = 3 * (p4 - p2) - Cy;
	double Ay = 1 - Cy - By;

	FindXFor(m, Cx, Bx, Ax);

	return BezierY(FindXFor(m, Cx, Bx, Ax), Cy, By, Ay);
}

inline double IPlugAnimation::LinearInterpolation(double p)
{
	return p;
}

inline double IPlugAnimation::QuadraticEaseIn(double p)
{
	return p * p;
}

inline double IPlugAnimation::QuadraticEaseOut(double p)
{
	return -(p * (p - 2));
}

inline double IPlugAnimation::QuadraticEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 2 * p * p;
	}
	else
	{
		return (-2 * p * p) + (4 * p) - 1;
	}
}

inline double IPlugAnimation::CubicEaseIn(double p)
{
	return p * p * p;
}

inline double IPlugAnimation::CubicEaseOut(double p)
{
	double f = (p - 1);
	return f * f * f + 1;
}

inline double IPlugAnimation::CubicEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 4 * p * p * p;
	}
	else
	{
		double f = ((2 * p) - 2);
		return 0.5 * f * f * f + 1;
	}
}

inline double IPlugAnimation::QuarticEaseIn(double p)
{
	return p * p * p * p;
}

inline double IPlugAnimation::QuarticEaseOut(double p)
{
	double f = (p - 1);
	return f * f * f * (1 - p) + 1;
}

inline double IPlugAnimation::QuarticEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 8 * p * p * p * p;
	}
	else
	{
		double f = (p - 1);
		return -8 * f * f * f * f + 1;
	}
}

inline double IPlugAnimation::QuinticEaseIn(double p)
{
	return p * p * p * p * p;
}

inline double IPlugAnimation::QuinticEaseOut(double p)
{
	double f = (p - 1);
	return f * f * f * f * f + 1;
}

inline double IPlugAnimation::QuinticEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 16 * p * p * p * p * p;
	}
	else
	{
		double f = ((2 * p) - 2);
		return  0.5 * f * f * f * f * f + 1;
	}
}

inline double IPlugAnimation::SineEaseIn(double p)
{
	return sin((p - 1) * M_PI_2) + 1;
}

inline double IPlugAnimation::SineEaseOut(double p)
{
	return sin(p * M_PI_2);
}

inline double IPlugAnimation::SineEaseInOut(double p)
{
	return 0.5 * (1 - cos(p * M_PI));
}

inline double IPlugAnimation::CircularEaseIn(double p)
{
	return 1 - sqrt(1 - (p * p));
}

inline double IPlugAnimation::CircularEaseOut(double p)
{
	return sqrt((2 - p) * p);
}

inline double IPlugAnimation::CircularEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 0.5 * (1 - sqrt(1 - 4 * (p * p)));
	}
	else
	{
		return 0.5 * (sqrt(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
	}
}

inline double IPlugAnimation::ExponentialEaseIn(double p)
{
	return (p == 0.0) ? p : pow(2, 10 * (p - 1));
}

inline double IPlugAnimation::ExponentialEaseOut(double p)
{
	return (p == 1.0) ? p : 1 - pow(2, -10 * p);
}

inline double IPlugAnimation::ExponentialEaseInOut(double p)
{
	if (p == 0.0 || p == 1.0) return p;

	if (p < 0.5)
	{
		return 0.5 * pow(2, (20 * p) - 10);
	}
	else
	{
		return -0.5 * pow(2, (-20 * p) + 10) + 1;
	}
}

inline double IPlugAnimation::ElasticEaseIn(double p)
{
	return sin(13 * M_PI_2 * p) * pow(2, 10 * (p - 1));
}

inline double IPlugAnimation::ElasticEaseOut(double p)
{
	return sin(-13 * M_PI_2 * (p + 1)) * pow(2, -10 * p) + 1;
}

inline double IPlugAnimation::ElasticEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 0.5 * sin(13 * M_PI_2 * (2 * p)) * pow(2, 10 * ((2 * p) - 1));
	}
	else
	{
		return 0.5 * (sin(-13 * M_PI_2 * ((2 * p - 1) + 1)) * pow(2, -10 * (2 * p - 1)) + 2);
	}
}

inline double IPlugAnimation::BackEaseIn(double p)
{
	return p * p * p - p * sin(p * M_PI);
}

inline double IPlugAnimation::BackEaseOut(double p)
{
	double f = (1 - p);
	return 1 - (f * f * f - f * sin(f * M_PI));
}

inline double IPlugAnimation::BackEaseInOut(double p)
{
	if (p < 0.5)
	{
		double f = 2 * p;
		return 0.5 * (f * f * f - f * sin(f * M_PI));
	}
	else
	{
		double f = (1 - (2 * p - 1));
		return 0.5 * (1 - (f * f * f - f * sin(f * M_PI))) + 0.5;
	}
}

inline double IPlugAnimation::BounceEaseIn(double p)
{
	return 1 - BounceEaseOut(1 - p);
}

inline double IPlugAnimation::BounceEaseOut(double p)
{
	if (p < 4 / 11.0)
	{
		return (121 * p * p) / 16.0;
	}
	else if (p < 8 / 11.0)
	{
		return (363 / 40.0 * p * p) - (99 / 10.0 * p) + 17 / 5.0;
	}
	else if (p < 9 / 10.0)
	{
		return (4356 / 361.0 * p * p) - (35442 / 1805.0 * p) + 16061 / 1805.0;
	}
	else
	{
		return (54 / 5.0 * p * p) - (513 / 25.0 * p) + 268 / 25.0;
	}
}

inline double IPlugAnimation::BounceEaseInOut(double p)
{
	if (p < 0.5)
	{
		return 0.5 * BounceEaseIn(p * 2);
	}
	else
	{
		return 0.5 * BounceEaseOut(p * 2 - 1) + 0.5;
	}
}
