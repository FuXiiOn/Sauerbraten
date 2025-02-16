#include "geom.h"

//NOT USING ALL OF THIS YET xd


float Vector2::length()
{
	float length = sqrt(pow(x, 2.0f) + pow(y, 2.0f));
	return length;
}

Vector2 Vector2::normalize()
{
	Vector2 result(*this);

	while (result.x > 180.0f) result.x -= 360.0f;
	while (result.x < -180.0f) result.x += 360.0f;

	while (result.y > 90.0f) result.y -= 180.0f;
	while (result.y < -90.0f) result.y += 180.0f;

	return result;
}

Vector2 calcAngle(Vector3 posSrc, Vector3 posDst)
{
	Vector3 delta = posSrc - posDst;
	
	float pitch = -atan(delta.z / sqrt(pow(delta.x, 2.0f) + pow(delta.y, 2.0f))) * 180 / PI;

	float yaw = atan2(delta.x, -delta.y) * 180 / PI +180.0f;

	return Vector2(yaw, pitch);
}
