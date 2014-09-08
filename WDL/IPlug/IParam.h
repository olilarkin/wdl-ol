#ifndef _IPARAM_
#define _IPARAM_

#include "Containers.h"
#include <math.h>

#define MAX_PARAM_NAME_LEN 32 // e.g. "Gain"
#define MAX_PARAM_LABEL_LEN 32 // e.g. "Percent"
#define MAX_PARAM_DISPLAY_LEN 32 // e.g. "100" / "Mute"
#define MAX_PARAM_DISPLAY_PRECISION 6

inline double ToNormalizedParam(double nonNormalizedValue, double min, double max, double shape)
{
  return pow((nonNormalizedValue - min) / (max - min), 1.0 / shape);
}

inline double FromNormalizedParam(double normalizedValue, double min, double max, double shape)
{
  return min + pow((double) normalizedValue, shape) * (max - min);
}

class IParam
{
public:
  enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };

  IParam();
  ~IParam();

  EParamType Type() { return mType; }

  void InitBool(const char* name, bool defaultVal, const char* label = "", const char* group = ""); // LABEL not used here
  void InitEnum(const char* name, int defaultVal, int nEnums, const char* label = "", const char* group = ""); // LABEL not used here
  void InitInt(const char* name, int defaultVal, int minVal, int maxVal, const char* label = "", const char* group = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", const char* group = "", double shape = 1.);

  void Set(double value) { mValue = BOUNDED(value, mMin, mMax); }
  void SetDisplayText(int value, const char* text);
  void SetCanAutomate(bool canAutomate) { mCanAutomate = canAutomate; }
  void SetIgnorePresets(bool ignorePresets = true) { mIgnorePresets = ignorePresets; }
  // The higher the shape, the more resolution around host value zero.
  void SetShape(double shape);

  void SetToDefault() { mValue = mDefault; }

  // Call this if your param is (x, y) but you want to always display (-x, -y).
  void NegateDisplay() { mNegateDisplay = true; }
  bool DisplayIsNegated() const { return mNegateDisplay; }

  //call this to make sure the param display text allways has a sign
  void SignDisplay() { mSignDisplay = true; }

  // Accessors / converters.
  // These all return the readable value, not the VST (0,1).
  double Value() const { return mValue; }
  bool Bool() const { return (mValue >= 0.5); }
  int Int() const { return int(mValue); }
  double DBToAmp();

  void SetNormalized(double normalizedValue);
  double GetNormalized();
  double GetNormalized(double nonNormalizedValue);
  void GetDisplayForHost(char* rDisplay) { GetDisplayForHost(mValue, false, rDisplay); }
  void GetDisplayForHostNoDisplayText(char* rDisplay) { GetDisplayForHost(mValue, false, rDisplay, false); }
  void GetDisplayForHost(double value, bool normalized, char* rDisplay, bool withDisplayText = true);
  const char* GetNameForHost();
  const char* GetLabelForHost();
  const char* GetParamGroupForHost();

  int GetNDisplayTexts();
  const char* GetDisplayText(int value);
  const char* GetDisplayTextAtIdx(int idx, int* value = 0);
  bool MapDisplayText(char* str, int* pValue);  // Reverse map back to value.
  void GetBounds(double* pMin, double* pMax);
  const double GetShape() {return mShape;}
  const double GetStep() {return mStep;}
  const double GetDefault() {return mDefault;}
  const double GetDefaultNormalized() {return ToNormalizedParam(mDefault, mMin, mMax, mShape);}
  const double GetMin() {return mMin;}
  const double GetMax() {return mMax;}
  const double GetRange() {return mMax - mMin;}
  const int GetPrecision() {return mDisplayPrecision;}

  bool GetCanAutomate() { return mCanAutomate; }
  bool GetIgnorePresets() { return mIgnorePresets; }

private:
  // All we store is the readable values.
  // SetFromHost() and GetForHost() handle conversion from/to (0,1).
  EParamType mType;
  double mValue, mMin, mMax, mStep, mShape, mDefault;
  int mDisplayPrecision;
  char mName[MAX_PARAM_NAME_LEN];
  char mLabel[MAX_PARAM_LABEL_LEN];
  char mParamGroup[MAX_PARAM_LABEL_LEN];
  bool mNegateDisplay;
  bool mSignDisplay;
  bool mCanAutomate;
  bool mIgnorePresets;

  struct DisplayText
  {
    int mValue;
    char mText[MAX_PARAM_DISPLAY_LEN];
  };

  WDL_TypedBuf<DisplayText> mDisplayTexts;
};

struct IParamUID
{
    IParamUID()
    {
        mCurrentID = 0;
        mInitialized = false;
        mNInited = 0;
    }
    bool Init(int numParams)
    {
        if (mInitialized) { return false; }
        // need initialization
        assert(numParams > 0);
        mUID.Resize(numParams);
        int i = 0;
        while (i < mUID.GetSize())
        {
            mUID.Get()[i] = -1;
            ++i;
        }
        return true;
        // now the Plug should start describing (setting) the params one by one and their unique IDs!
    }

    //void Set(int paramIdx, int UID = -1)
    void Set(int paramIdx) { Set(-1, paramIdx); }
    void Set(int UID, int paramIdx)
    {
        assert(mInitialized == false);
        // UID "-1" == autoincrement
        if (paramIdx < 0 || paramIdx >= mUID.GetSize()) { return; }
        int *p = &(mUID.Get()[paramIdx]);
        if (*p == -1)
        {
            if (UID == -1) { UID = mCurrentID; }
            mCurrentID = UID+1;
            *p = UID;
            ++mNInited;
            if (mNInited == mUID.GetSize())
            {
                // last one
                mInitialized = true;
            }
            return;
        }
    }
    void Rem()
    {
        // when deleting existing params - you can replace their Set(paramIdx) call with a Rem(); // paramIdx
        // this simply autoincrements
        assert(mInitialized == false);
        ++mCurrentID;
    }
    int FindIDX(int UID)
    {
        return mUID.Find(UID);
    }
    int GetUID(int paramIdx)
    {
        return mUID.Get()[paramIdx];
    }

    WDL_TypedBuf<int> mUID;
    int mCurrentID;
    int mNInited;
    bool mInitialized;
};

#endif
