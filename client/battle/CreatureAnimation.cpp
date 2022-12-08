/*
 * CCreatureAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CreatureAnimation.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"

#include "../gui/Canvas.h"

static const SDL_Color creatureBlueBorder = { 0, 255, 255, 255 };
static const SDL_Color creatureGoldBorder = { 255, 255, 0, 255 };
static const SDL_Color creatureNoBorder  =  { 0, 0, 0, 0 };

SDL_Color AnimationControls::getBlueBorder()
{
	return creatureBlueBorder;
}

SDL_Color AnimationControls::getGoldBorder()
{
	return creatureGoldBorder;
}

SDL_Color AnimationControls::getNoBorder()
{
	return creatureNoBorder;
}

std::shared_ptr<CreatureAnimation> AnimationControls::getAnimation(const CCreature * creature)
{
	auto func = std::bind(&AnimationControls::getCreatureAnimationSpeed, creature, _1, _2);
	return std::make_shared<CreatureAnimation>(creature->animDefName, func);
}

float AnimationControls::getCreatureAnimationSpeed(const CCreature * creature, const CreatureAnimation * anim, size_t group)
{
	ECreatureAnimType::Type type = ECreatureAnimType::Type(group);

	assert(creature->animation.walkAnimationTime != 0);
	assert(creature->animation.attackAnimationTime != 0);
	assert(anim->framesInGroup(type) != 0);

	// possible new fields for creature format:
	//split "Attack time" into "Shoot Time" and "Cast Time"

	// a lot of arbitrary multipliers, mostly to make animation speed closer to H3
	const float baseSpeed = 0.1f;
	const float speedMult = static_cast<float>(settings["battle"]["animationSpeed"].Float());
	const float speed = baseSpeed / speedMult;

	switch (type)
	{
	case ECreatureAnimType::MOVING:
		return static_cast<float>(speed * 2 * creature->animation.walkAnimationTime / anim->framesInGroup(type));

	case ECreatureAnimType::MOUSEON:
		return baseSpeed;
	case ECreatureAnimType::HOLDING:
		return static_cast<float>(baseSpeed * creature->animation.idleAnimationTime / anim->framesInGroup(type));

	case ECreatureAnimType::SHOOT_UP:
	case ECreatureAnimType::SHOOT_FRONT:
	case ECreatureAnimType::SHOOT_DOWN:
	case ECreatureAnimType::CAST_UP:
	case ECreatureAnimType::CAST_FRONT:
	case ECreatureAnimType::CAST_DOWN:
	case ECreatureAnimType::VCMI_CAST_DOWN:
	case ECreatureAnimType::VCMI_CAST_FRONT:
	case ECreatureAnimType::VCMI_CAST_UP:
		return static_cast<float>(speed * 4 * creature->animation.attackAnimationTime / anim->framesInGroup(type));

	// as strange as it looks like "attackAnimationTime" does not affects melee attacks
	// necessary because length of these animations must be same for all creatures for synchronization
	case ECreatureAnimType::ATTACK_UP:
	case ECreatureAnimType::ATTACK_FRONT:
	case ECreatureAnimType::ATTACK_DOWN:
	case ECreatureAnimType::HITTED:
	case ECreatureAnimType::DEFENCE:
	case ECreatureAnimType::DEATH:
	case ECreatureAnimType::DEATH_RANGED:
	case ECreatureAnimType::RESURRECTION:
	case ECreatureAnimType::VCMI_2HEX_DOWN:
	case ECreatureAnimType::VCMI_2HEX_FRONT:
	case ECreatureAnimType::VCMI_2HEX_UP:
		return speed * 3 / anim->framesInGroup(type);

	case ECreatureAnimType::TURN_L:
	case ECreatureAnimType::TURN_R:
		return speed / 3;

	case ECreatureAnimType::MOVE_START:
	case ECreatureAnimType::MOVE_END:
		return speed / 3;

	case ECreatureAnimType::DEAD:
	case ECreatureAnimType::DEAD_RANGED:
		return speed;

	default:
		return speed;
	}
}

float AnimationControls::getProjectileSpeed()
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 100);
}

float AnimationControls::getCatapultSpeed()
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 20);
}

float AnimationControls::getSpellEffectSpeed()
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 30);
}

float AnimationControls::getMovementDuration(const CCreature * creature)
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 4 / creature->animation.walkAnimationTime);
}

float AnimationControls::getFlightDistance(const CCreature * creature)
{
	return static_cast<float>(creature->animation.flightAnimationDistance * 200);
}

ECreatureAnimType::Type CreatureAnimation::getType() const
{
	return type;
}

void CreatureAnimation::setType(ECreatureAnimType::Type type)
{
	this->type = type;
	currentFrame = 0;
	once = false;

	play();
}

void CreatureAnimation::shiftColor(const ColorShifter* shifter)
{
	if(forward)
		forward->shiftColor(shifter);

	if(reverse)
		reverse->shiftColor(shifter);
}

CreatureAnimation::CreatureAnimation(const std::string & name_, TSpeedController controller)
	: name(name_),
	  speed(0.1f),
	  currentFrame(0),
	  elapsedTime(0),
	  type(ECreatureAnimType::HOLDING),
	  border(CSDL_Ext::makeColor(0, 0, 0, 0)),
	  speedController(controller),
	  once(false)
{
	forward = std::make_shared<CAnimation>(name_);
	reverse = std::make_shared<CAnimation>(name_);

	//todo: optimize
	forward->preload();
	reverse->preload();

	// if necessary, add one frame into vcmi-only group DEAD
	if(forward->size(ECreatureAnimType::DEAD) == 0)
	{
		forward->duplicateImage(ECreatureAnimType::DEATH, forward->size(ECreatureAnimType::DEATH)-1, ECreatureAnimType::DEAD);
		reverse->duplicateImage(ECreatureAnimType::DEATH, reverse->size(ECreatureAnimType::DEATH)-1, ECreatureAnimType::DEAD);
	}

	if(forward->size(ECreatureAnimType::DEAD_RANGED) == 0 && forward->size(ECreatureAnimType::DEATH_RANGED) != 0)
	{
		forward->duplicateImage(ECreatureAnimType::DEATH_RANGED, forward->size(ECreatureAnimType::DEATH_RANGED)-1, ECreatureAnimType::DEAD_RANGED);
		reverse->duplicateImage(ECreatureAnimType::DEATH_RANGED, reverse->size(ECreatureAnimType::DEATH_RANGED)-1, ECreatureAnimType::DEAD_RANGED);
	}

	if(forward->size(ECreatureAnimType::RESURRECTION) == 0)
	{
		for (size_t i = 0; i < forward->size(ECreatureAnimType::DEATH); ++i)
		{
			size_t current = forward->size(ECreatureAnimType::DEATH) - 1 - i;

			forward->duplicateImage(ECreatureAnimType::DEATH, current, ECreatureAnimType::RESURRECTION);
			reverse->duplicateImage(ECreatureAnimType::DEATH, current, ECreatureAnimType::RESURRECTION);
		}
	}

	//TODO: get dimensions form CAnimation
	auto first = forward->getImage(0, type, true);

	if(!first)
	{
		fullWidth = 0;
		fullHeight = 0;
		return;
	}
	fullWidth = first->width();
	fullHeight = first->height();

	reverse->verticalFlip();

	play();
}

void CreatureAnimation::endAnimation()
{
	once = false;
	auto copy = onAnimationReset;
	onAnimationReset.clear();
	copy();
}

bool CreatureAnimation::incrementFrame(float timePassed)
{
	elapsedTime += timePassed;
	currentFrame += timePassed * speed;
	const auto framesNumber = framesInGroup(type);

	if(framesNumber <= 0)
	{
		endAnimation();
	}
	else if(currentFrame >= float(framesNumber))
	{
		// just in case of extremely low fps (or insanely high speed)
		while(currentFrame >= float(framesNumber))
			currentFrame -= framesNumber;

		if(once)
			setType(ECreatureAnimType::HOLDING);

		endAnimation();
		return true;
	}
	return false;
}

void CreatureAnimation::setBorderColor(SDL_Color palette)
{
	border = palette;
}

int CreatureAnimation::getWidth() const
{
	return fullWidth;
}

int CreatureAnimation::getHeight() const
{
	return fullHeight;
}

float CreatureAnimation::getCurrentFrame() const
{
	return currentFrame;
}

void CreatureAnimation::playOnce( ECreatureAnimType::Type type )
{
	setType(type);
	once = true;
}

inline int getBorderStrength(float time)
{
	float borderStrength = fabs(vstd::round(time) - time) * 2; // generate value in range 0-1

	return static_cast<int>(borderStrength * 155 + 100); // scale to 0-255
}

static SDL_Color genShadow(ui8 alpha)
{
	return CSDL_Ext::makeColor(0, 0, 0, alpha);
}

static SDL_Color genBorderColor(ui8 alpha, const SDL_Color & base)
{
	return CSDL_Ext::makeColor(base.r, base.g, base.b, ui8(base.a * alpha / 256));
}

static ui8 mixChannels(ui8 c1, ui8 c2, ui8 a1, ui8 a2)
{
	return c1*a1 / 256 + c2*a2*(255 - a1) / 256 / 256;
}

static SDL_Color addColors(const SDL_Color & base, const SDL_Color & over)
{
	return CSDL_Ext::makeColor(
			mixChannels(over.r, base.r, over.a, base.a),
			mixChannels(over.g, base.g, over.a, base.a),
			mixChannels(over.b, base.b, over.a, base.a),
			ui8(over.a + base.a * (255 - over.a) / 256)
			);
}

void CreatureAnimation::genBorderPalette(IImage::BorderPallete & target)
{
	target[0] = genBorderColor(getBorderStrength(elapsedTime), border);
	target[1] = addColors(genShadow(128), genBorderColor(getBorderStrength(elapsedTime), border));
	target[2] = addColors(genShadow(64),  genBorderColor(getBorderStrength(elapsedTime), border));
}

void CreatureAnimation::nextFrame(Canvas & canvas, bool facingRight)
{
	size_t frame = static_cast<size_t>(floor(currentFrame));

	std::shared_ptr<IImage> image;

	if(facingRight)
		image = forward->getImage(frame, type);
	else
		image = reverse->getImage(frame, type);

	if(image)
	{
		IImage::BorderPallete borderPallete;
		genBorderPalette(borderPallete);

		image->setBorderPallete(borderPallete);

		canvas.draw(image, pos.topLeft(), Rect(0, 0, pos.w, pos.h));
	}
}

int CreatureAnimation::framesInGroup(ECreatureAnimType::Type group) const
{
	return static_cast<int>(forward->size(group));
}

bool CreatureAnimation::isDead() const
{
	return getType() == ECreatureAnimType::DEAD
		|| getType() == ECreatureAnimType::DEAD_RANGED;
}

bool CreatureAnimation::isDying() const
{
	return getType() == ECreatureAnimType::DEATH
		|| getType() == ECreatureAnimType::DEATH_RANGED;
}

bool CreatureAnimation::isDeadOrDying() const
{
	return getType() == ECreatureAnimType::DEAD
		|| getType() == ECreatureAnimType::DEATH
		|| getType() == ECreatureAnimType::DEAD_RANGED
		|| getType() == ECreatureAnimType::DEATH_RANGED;
}

bool CreatureAnimation::isIdle() const
{
	return getType() == ECreatureAnimType::HOLDING
		|| getType() == ECreatureAnimType::MOUSEON;
}

bool CreatureAnimation::isMoving() const
{
	return getType() == ECreatureAnimType::MOVE_START
		|| getType() == ECreatureAnimType::MOVING
		|| getType() == ECreatureAnimType::MOVE_END
		|| getType() == ECreatureAnimType::TURN_L
		|| getType() == ECreatureAnimType::TURN_R;
}

bool CreatureAnimation::isShooting() const
{
	return getType() == ECreatureAnimType::SHOOT_UP
		|| getType() == ECreatureAnimType::SHOOT_FRONT
		|| getType() == ECreatureAnimType::SHOOT_DOWN;
}

void CreatureAnimation::pause()
{
	speed = 0;
}

void CreatureAnimation::play()
{
	//logAnim->trace("Play %s group %d at %d:%d", name, static_cast<int>(getType()), pos.x, pos.y);
    speed = 0;
    if(speedController(this, type) != 0)
        speed = 1 / speedController(this, type);
}
