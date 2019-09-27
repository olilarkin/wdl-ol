#ifndef _IVECSLIDERS_
#define _IVECSLIDERS_

#ifdef WIN32
#define round(x) floor(x + 0.5)
#endif


#define MAXSLIDERS 512

class MultiSliderControlV: public IControl
{
public:
  MultiSliderControlV(IPlugBase *pPlug,
                      IRECT pR,
                      int paramIdx,
                      int numSliders,
                      int handleWidth,
                      const IColor* bgcolor,
                      const IColor* fgcolor,
                      const IColor* hlcolor)
    : IControl(pPlug, pR, paramIdx)
  {
    mBgColor = *bgcolor;
    mFgColor = *fgcolor;
    mHlColor = *hlcolor;

    mNumSliders = numSliders;
    mHighlighted = -1;
    mGrain = 0.001;
    mSliderThatChanged = -1;

    float sliderWidth = floor((float) mRECT.W() / (float) numSliders);

    mSteps = new double[numSliders];

    for(int i=0; i<numSliders; i++)
    {
      int lpos = (i * sliderWidth);
      mSteps[i] = 0.;

      mSliderBounds[i] = new IRECT(mRECT.L + lpos , mRECT.T, mRECT.L + lpos + sliderWidth, mRECT.B);
    }

    mHandleWidth = handleWidth;
  }

  ~MultiSliderControlV()
  {
    delete [] mSteps;

    for(int i=0; i<mNumSliders; i++)
    {
      delete mSliderBounds[i];
    }
  }

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&mBgColor, &mRECT);

    for(int i=0; i<mNumSliders; i++)
    {
      float yPos = mSteps[i] * mRECT.H();
      int top = mRECT.B - yPos;
      //int bottom = top + 10;
      int bottom = mRECT.B;

      IColor * color = &mFgColor;
      if(i == mHighlighted) color = &mHlColor;

      IRECT srect = IRECT(mSliderBounds[i]->L, top, mSliderBounds[i]->R-1, bottom);
      pGraphics->FillIRect(color, &srect );
    }

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void OnMouseUp(int x, int y, IMouseMod* pMod)
  {
    //TODO: check this isn't going to cause problems... this will happen from the gui thread
    mPlug->ModifyCurrentPreset();
    mPlug->DirtyPTCompareState();
  }

  void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void SnapToMouse(int x, int y)
  {
    x = BOUNDED(x, mRECT.L, mSliderBounds[mNumSliders-1]->R-1);
    y = BOUNDED(y, mRECT.T, mRECT.B-1);

    float yValue =  (float) (y-mRECT.T) / (float) mRECT.H();

    yValue = round( yValue / mGrain ) * mGrain;

    int sliderTest = mNumSliders-1;
    bool foundIntersection = false;

    while (!foundIntersection)
    {
      foundIntersection = mSliderBounds[sliderTest]->Contains(x, y);

      if (!foundIntersection && sliderTest !=0 ) sliderTest--;
    }

    if (foundIntersection)
    {
      //mHighlighted = sliderTest;
      mSteps[sliderTest] = 1. - BOUNDED(yValue, 0., 1.);
      mSliderThatChanged = sliderTest;
      mPlug->OnParamChange(mParamIdx); // TODO: rethink this WRT threading
    }
    else
    {
      mSliderThatChanged = -1;
      //mHighlighted = -1;
    }

    SetDirty();
  }

  void GetLatestChange(double* data)
  {
    data[mSliderThatChanged] = mSteps[mSliderThatChanged];
  }

  void GetState(double* data)
  {
    memcpy( data, mSteps, mNumSliders * sizeof(double));
  }

  void SetState(double* data)
  {
    memcpy(mSteps, data, mNumSliders * sizeof(double));

    SetDirty();
  }

  void SetHighlight(int i)
  {
    mHighlighted = i;

    SetDirty();
  }

private:
  IColor mBgColor, mFgColor, mHlColor;
  int mNumSliders;
  int mHandleWidth;
  int mSliderThatChanged;
  double *mSteps;
  double mGrain;
  IRECT *mSliderBounds[MAXSLIDERS];
  int mHighlighted;
};


class IVSliderControl: public IControl
{
public:
  IVSliderControl(IPlugBase *pPlug,
                  IRECT pR,
                  int paramIdx,
                  int handleWidth,
                  const IColor* bgcolor,
                  const IColor* fgcolor)
    : IControl(pPlug, pR, paramIdx)
  {
    mBgColor = *bgcolor;
    mFgColor = *fgcolor;
    mHandleWidth = handleWidth;
  }

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&mBgColor, &mRECT);

    float yPos = mValue * mRECT.H();
    int top = mRECT.B - yPos;

    IRECT innerRect = IRECT(mRECT.L+2, top, mRECT.R-2, mRECT.B);
    pGraphics->FillIRect(&mFgColor, &innerRect);

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void SnapToMouse(int x, int y)
  {
    x = BOUNDED(x, mRECT.L, mRECT.R-1);
    y = BOUNDED(y, mRECT.T, mRECT.B-1);

    float yValue = 1. - (float) (y-mRECT.T) / (float) mRECT.H();

    mValue = round( yValue / 0.001 ) * 0.001;

    mPlug->SetParameterFromGUI(mParamIdx, BOUNDED(mValue, 0., 1.));

    SetDirty();
  }

private:
  IColor mBgColor, mFgColor;
  int mHandleWidth;
};

/*For making presets during development- Creates a Rectangle with a triangle on it that opens a menu
to load or save presets and banks. Also, there are options to move forward and back through presets and
to dump preset source code to a text file. pParameterNames is a pointer to an array holding the parameter names. */
class PresetFileMenuDev : public IPanelControl
{
public:
  PresetFileMenuDev(IPlugBase *pPlug, IRECT pR, const char** pParameterNames, const IColor* pRectBgColor = &COLOR_WHITE, const IColor* pTriBgColor = &COLOR_GRAY)
    : IPanelControl(pPlug, pR, &COLOR_BLUE)
    , mParamNames(pParameterNames)
    , mRectColor(*pRectBgColor)
    , mTriColor(*pTriBgColor)
  {}

  ~PresetFileMenuDev() {}

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&mRectColor, &mRECT);

    int ax = mRECT.R - 8;
    int ay = mRECT.T + 4;
    int bx = ax + 4;
    int by = ay;
    int cx = ax + 2;
    int cy = ay + 2;

    pGraphics->FillTriangle(&mTriColor, ax, ay, bx, by, cx, cy, &mBlend);

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->L)
    {
      doPopupMenu();
    }

    Redraw(); // seems to need this
    SetDirty();
  }

  void doPopupMenu()
  {
    IPopupMenu menu;

    IGraphics* gui = mPlug->GetGUI();

    menu.AddItem("Previous preset");
    menu.AddItem("Next preset");
    menu.AddSeparator();
    menu.AddItem("Save Program...");
    menu.AddItem("Save Bank...");
    menu.AddSeparator();
    menu.AddItem("Load Program...");
    menu.AddItem("Load Bank...");
    menu.AddSeparator();
    menu.AddItem("Dump MakePresetFromNamedParams");
    menu.AddItem("Dump MakePreset");
    menu.AddItem("Dump MakePresetBlob");
    menu.AddItem("DumpBankBlob");

    if (gui->CreateIPopupMenu(&menu, &mRECT))
    {
      int itemChosen = menu.GetChosenItemIdx();
      WDL_String fileName;
      int numpresets = mPlug->NPresets();
      int currpreset = mPlug->GetCurrentPresetIdx();
      int prevpreset = (currpreset == 0) ? numpresets - 1 : currpreset - 1;
      int nextpreset = (currpreset == numpresets - 1) ? 0 : currpreset + 1;

      switch (itemChosen)
      {
      case 0://Previous preset
        mPlug->RestorePreset(prevpreset);
        mPlug->InformHostOfProgramChange();
        mPlug->DirtyParameters();
        break;
      case 1://Next preset
        mPlug->RestorePreset(nextpreset);
        mPlug->InformHostOfProgramChange();
        mPlug->DirtyParameters();
        break;
      case 3: //Save Program
        fileName.Set(mPlug->GetPresetName(currpreset));
        GetGUI()->PromptForFile(&fileName, kFileSave, &mPreviousPath, "fxp");
        mPlug->SaveProgramAsFXP(&fileName);
        break;
      case 4: //Save Bank
        fileName.Set("IPlugChunksBank");
        GetGUI()->PromptForFile(&fileName, kFileSave, &mPreviousPath, "fxb");
        mPlug->SaveBankAsFXB(&fileName);
        break;
      case 6: //Load Preset
        GetGUI()->PromptForFile(&fileName, kFileOpen, &mPreviousPath, "fxp");
        mPlug->LoadProgramFromFXP(&fileName);
        break;
      case 7: // Load Bank
        GetGUI()->PromptForFile(&fileName, kFileOpen, &mPreviousPath, "fxb");
        mPlug->LoadBankFromFXB(&fileName);
        break;
      case 9: //Dumps/adds MakePresetFromNamedParams to file
        fileName.Set("presets");
        GetGUI()->PromptForFile(&fileName, kFileSave, &mPreviousPath, "txt");
        mPlug->AppendPrstSrcNamed(fileName.Get(), mParamNames);
        break;
      case 10: //Dumps/adds MakePreset to file
        fileName.Set("presets");
        GetGUI()->PromptForFile(&fileName, kFileSave, &mPreviousPath, "txt");
        mPlug->AppendPrstSrc(fileName.Get());
        break;
      case 11: //Dumps/adds MakePresetFromBlob to file
        fileName.Set("blobpresets");
        GetGUI()->PromptForFile(&fileName, kFileSave, &mPreviousPath, "txt");
        mPlug->AppendPresetBlob(fileName.Get());
        break;
      case 12: // Dumps Bank of Blob presets to file
        fileName.Set("blobBank");
        GetGUI()->PromptForFile(&fileName, kFileSave, &mPreviousPath, "txt");
        mPlug->DumpBankBlob(fileName.Get());
      default:
        break;
      }
    }
  }
private:
  WDL_String mPreviousPath;
  IColor mRectColor;
  IColor mTriColor;
  const char** mParamNames;
};

/*Creates a rectangle that opens a menu with submenus for selecting presets. Supports up to 128 presets
in up to 8 submenus depending on the number of presets. pSubmenuNames is a pointer to an array of SubMenu names.*/
class PresetSubMenu : public IControl
{
private:
  WDL_String mDisp;
  IColor mColor;
  const char** pSubmenuNames;

public:
  PresetSubMenu(IPlugBase *pPlug, IRECT pR, IText* pText, const char** pSubmenuNames, const IColor* pRectBgColor = &COLOR_WHITE)
    : IControl(pPlug, pR, -1)
    , mColor(*pRectBgColor)
    , pSubmenuNames(pSubmenuNames)
  {
    mTextEntryLength = MAX_PRESET_NAME_LEN - 3;
    mText = *pText;
  }

  ~PresetSubMenu() {}

  bool Draw(IGraphics* pGraphics)
  {
    int pNumber = mPlug->GetCurrentPresetIdx();
    mDisp.SetFormatted(32, "%02d: %s", pNumber + 1, mPlug->GetPresetName(pNumber));

    pGraphics->FillIRect(&mColor, &mRECT);

    if (CSTR_NOT_EMPTY(mDisp.Get()))
    {
      return pGraphics->DrawIText(&mText, mDisp.Get(), &mRECT);
    }

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->R)
    {
      const char* pname = mPlug->GetPresetName(mPlug->GetCurrentPresetIdx());
      mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, pname);
    }
    else
    {
      doPopupMenu();
    }

    Redraw(); // seems to need this
    SetDirty();
  }

  void doPopupMenu()
  {
    IPopupMenu mMainMenu, mSubMenu1, mSubMenu2, mSubMenu3, mSubMenu4, mSubMenu5, mSubMenu6, mSubMenu7, mSubMenu8;
    int numItems = mPlug->NPresets();
    int nItemsInLastSubMenu = numItems % 16;//Assuming no more than 128 presets in 8 banks (16 presets max per submenu).
    int nSubMenus = (nItemsInLastSubMenu > 0) ? (numItems / 16) + 1 : numItems / 16;
    int currentPresetIdx = mPlug->GetCurrentPresetIdx();
    //Add presets
    for (int i = 0; i < numItems; i++)
    {
      const char* str = mPlug->GetPresetName(i);
      if (i < 16)
      {
        if (i == currentPresetIdx)
          mSubMenu1.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu1.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 16 && i < 32)
      {
        if (i == currentPresetIdx)
          mSubMenu2.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu2.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 32 && i < 48)
      {
        if (i == currentPresetIdx)
          mSubMenu3.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu3.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 48 && i < 64)
      {
        if (i == currentPresetIdx)
          mSubMenu4.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu4.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 64 && i < 80)
      {
        if (i == currentPresetIdx)
          mSubMenu5.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu5.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 80 && i < 96)
      {
        if (i == currentPresetIdx)
          mSubMenu6.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu6.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 96 && i < 112)
      {
        if (i == currentPresetIdx)
          mSubMenu7.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu7.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
      if (i >= 112 && i < 128)
      {
        if (i == currentPresetIdx)
          mSubMenu8.AddItem(str, -1, IPopupMenuItem::kChecked);
        else
          mSubMenu8.AddItem(str, -1, IPopupMenuItem::kNoFlags);
      }
    }
    //Add Banks
    if (nSubMenus > 0) mMainMenu.AddItem(pSubmenuNames[0], &mSubMenu1);//"sub menu1"
    if (nSubMenus > 1) mMainMenu.AddItem(pSubmenuNames[1], &mSubMenu2);//"sub menu2"
    if (nSubMenus > 2) mMainMenu.AddItem(pSubmenuNames[2], &mSubMenu3);//"sub menu3"
    if (nSubMenus > 3) mMainMenu.AddItem(pSubmenuNames[3], &mSubMenu4);//"sub menu4"
    if (nSubMenus > 4) mMainMenu.AddItem(pSubmenuNames[4], &mSubMenu5);//"sub menu5"
    if (nSubMenus > 5) mMainMenu.AddItem(pSubmenuNames[5], &mSubMenu6);//"sub menu6"
    if (nSubMenus > 6) mMainMenu.AddItem(pSubmenuNames[6], &mSubMenu7);//"sub menu7"
    if (nSubMenus == 8) mMainMenu.AddItem(pSubmenuNames[7], &mSubMenu8);//"sub menu8"

    IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&mMainMenu, &mRECT);
    int itemChosen = currentPresetIdx;

    if (selectedMenu == &mSubMenu1) itemChosen = selectedMenu->GetChosenItemIdx();
    if (selectedMenu == &mSubMenu2) itemChosen = selectedMenu->GetChosenItemIdx() + 16;
    if (selectedMenu == &mSubMenu3) itemChosen = selectedMenu->GetChosenItemIdx() + 32;
    if (selectedMenu == &mSubMenu4) itemChosen = selectedMenu->GetChosenItemIdx() + 48;
    if (selectedMenu == &mSubMenu5) itemChosen = selectedMenu->GetChosenItemIdx() + 64;
    if (selectedMenu == &mSubMenu6) itemChosen = selectedMenu->GetChosenItemIdx() + 80;
    if (selectedMenu == &mSubMenu7) itemChosen = selectedMenu->GetChosenItemIdx() + 96;
    if (selectedMenu == &mSubMenu8) itemChosen = selectedMenu->GetChosenItemIdx() + 112;

    if (itemChosen > -1)
    {
      mPlug->RestorePreset(itemChosen);
      mPlug->InformHostOfProgramChange();
      mPlug->DirtyParameters();
    }
  }

  void TextFromTextEntry(const char* txt)
  {
    WDL_String safeName;
    safeName.Set(txt, MAX_PRESET_NAME_LEN);

    mPlug->ModifyCurrentPreset(safeName.Get());
    mPlug->InformHostOfProgramChange();
    mPlug->DirtyParameters();
    SetDirty(false);
  }
  bool IsDirty() { return true; }
};

#endif _IVECSLIDERS_
