//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Ball.hxx"

enum Count: Int8 {
  renderCounterOffset = -4
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Ball::Ball(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::reset()
{
  myColor = myObjectColor = myDebugColor = 0;
  collision = myCollisionMaskDisabled;
  myIsEnabledOld = false;
  myIsEnabledNew = false;
  myIsEnabled = false;
  myIsDelaying = false;
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myEffectiveWidth = 1;
  myLastMovementTick = 0;
  myWidth = 1;
  myIsRendering = false;
  myDebugEnabled = false;
  myRenderCounter = 0;

  updateEnabled();
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enabl(uInt8 value)
{
  myIsEnabledNew = (value & 0x02) > 0;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::hmbl(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::resbl(uInt8 counter)
{
  myCounter = counter;

  myIsRendering = true;
  myRenderCounter = Count::renderCounterOffset + (counter - 157);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::ctrlpf(uInt8 value)
{
  static constexpr uInt8 ourWidths[] = {1, 2, 4, 8};

  myWidth = ourWidths[(value & 0x30) >> 4];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::vdelbl(uInt8 value)
{
  myIsDelaying = (value & 0x01) > 0;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setColor(uInt8 color)
{
  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setDebugColor(uInt8 color)
{
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enableDebugColors(bool enabled)
{
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::movementTick(uInt32 clock, bool apply)
{
  myLastMovementTick = myCounter;

  if (clock == myHmmClocks) myIsMoving = false;

  if (myIsMoving && apply) {
    render();
    tick(false);
  }

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::render()
{
  collision = (myIsRendering && myRenderCounter >= 0 && myIsEnabled) ?
    myCollisionMaskEnabled :
    myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::tick(bool isReceivingMclock)
{
  bool starfieldEffect = myIsMoving && isReceivingMclock;

  if (myCounter == 156) {
    myIsRendering = true;
    myRenderCounter = Count::renderCounterOffset;

    uInt8 starfieldDelta = (myCounter + 160 - myLastMovementTick) % 4;
    if (starfieldEffect && starfieldDelta == 3 && myWidth < 4) myRenderCounter++;

    switch (starfieldDelta) {
      case 3:
        myEffectiveWidth = myWidth == 1 ? 2 : myWidth;
        break;

      case 2:
        myEffectiveWidth = 0;
        break;

      default:
        myEffectiveWidth = myWidth;
        break;
    }

  } else if (myIsRendering && ++myRenderCounter >= (starfieldEffect ? myEffectiveWidth : myWidth))
    myIsRendering = false;

  if (++myCounter >= 160)
      myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::shuffleStatus()
{
  myIsEnabledOld = myIsEnabledNew;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::updateEnabled()
{
  myIsEnabled = !myIsSuppressed && (myIsDelaying ? myIsEnabledOld : myIsEnabledNew);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::applyColors()
{
  myColor = myDebugEnabled ? myDebugColor : myObjectColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    out.putInt(collision);
    out.putInt(myCollisionMaskDisabled);
    out.putInt(myCollisionMaskEnabled);

    out.putByte(myColor);
    out.putByte(myObjectColor);
    out.putByte(myDebugColor);
    out.putBool(myDebugEnabled);

    out.putBool(myIsEnabledOld);
    out.putBool(myIsEnabledNew);
    out.putBool(myIsEnabled);
    out.putBool(myIsSuppressed);
    out.putBool(myIsDelaying);

    out.putByte(myHmmClocks);
    out.putByte(myCounter);
    out.putBool(myIsMoving);
    out.putByte(myWidth);
    out.putByte(myEffectiveWidth);
    out.putByte(myLastMovementTick);

    out.putBool(myIsRendering);
    out.putByte(myRenderCounter);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Ball::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    collision = in.getInt();
    myCollisionMaskDisabled = in.getInt();
    myCollisionMaskEnabled = in.getInt();

    myColor = in.getByte();
    myObjectColor = in.getByte();
    myDebugColor = in.getByte();
    myDebugEnabled = in.getBool();

    myIsEnabledOld = in.getBool();
    myIsEnabledNew = in.getBool();
    myIsEnabled = in.getBool();
    myIsSuppressed = in.getBool();
    myIsDelaying = in.getBool();

    myHmmClocks = in.getByte();
    myCounter = in.getByte();
    myIsMoving = in.getBool();
    myWidth = in.getByte();
    myEffectiveWidth = in.getByte();
    myLastMovementTick = in.getByte();

    myIsRendering = in.getBool();
    myRenderCounter = in.getByte();

    updateEnabled();
    applyColors();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Ball::load" << endl;
    return false;
  }

  return true;
}
