ANIM_CONTROL_NAME="FriendsControl"

ANIM_STATES: 
	Run,
	Falling,
	JumpUp,
	Land,
	Roll,
	Idle(Idle0->NEXT, Idle1->NEXT);

ANIM_VARIABLES:
	VelocityHorzLen,
	VelocityVertValue,
	IsFalling,

SET SpeedShiftPoint = 0.05;
SET VeryHighFallSpeed = -6.0;

Idle -> Run: ConditionFloat(VelocityHorzLen >= SpeedShiftPoint);
Run -> Idle: ConditionFloat(VelocityHorzLen < SpeedShiftPoint);
ANY_STATE -> JumpUp: 
	ConditionBool(IsFalling == true) AND
	ConditionFloat(VelocityVertValue > SpeedShiftPoint);
ANY_STATE -> Falling (TIME_TO_TRANSIT=0.5, ANIM_SHOULD_FINISH=false):
	ConditionBool(IsFalling == true) AND
	ConditionFloat(VelocityVertValue < -SpeedShiftPoint);
