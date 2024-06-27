#ifndef CAM_H
#define CAM_H

class Cam
{
public:
	glm::vec3 pos;

	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 back;
public:
	Cam(const glm::vec3& pos)
		:pos{ pos }, right{ 1.0f, 0.0f, 0.0f }, up{ 0.0f, 1.0f, 0.0f }, back{ 0.0f, 0.0f, 1.0f }
	{
	}
	~Cam()
	{

	}

	void turnRel(const glm::vec3& axis, float angle)
	{
		glm::vec3 tempBack{ back };
		glm::vec3 tempRight{ right };
		float angle1{ (tempBack.x > 0 ? 1 : -1) * glm::acos(tempBack.z / glm::sqrt(tempBack.x * tempBack.x + tempBack.z * tempBack.z)) };
		glm::mat3 rot1{ glm::rotate(glm::mat4(1.0f), -angle1, glm::vec3(0.0f, 1.0f, 0.0f)) };
		tempBack = rot1 * tempBack;
		tempRight = rot1 * tempRight;
		float angle2{ (tempBack.y > 0 ? 1 : -1) * glm::acos(tempBack.z / glm::sqrt(tempBack.y * tempBack.y + tempBack.z * tempBack.z)) };
		glm::mat3 rot2{ glm::rotate(glm::mat4(1.0f), angle2, glm::vec3(1.0f, 0.0f, 0.0f)) };
		tempRight = rot2 * tempRight;
		float angle3{ (tempRight.y > 0 ? 1 : -1) * glm::acos(tempRight.x / glm::sqrt(tempRight.x * tempRight.x + tempRight.y * tempRight.y)) };

		rot1 = glm::rotate(glm::mat4(1.0f), angle1, glm::vec3(0.0f, 1.0f, 0.0f));
		rot2 = glm::rotate(glm::mat4(1.0f), -angle2, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat3 rot3{ glm::rotate(glm::mat4(1.0f), angle3, glm::vec3(0.0f, 0.0f, 1.0f)) };

		glm::mat3 mat{ glm::rotate(glm::mat4(1.0f), angle, glm::vec3(rot1 * rot2 * rot3 * glm::vec4(axis, 1.0f))) };
		right = mat * right;
		up = mat * up;
		back = mat * back;
	}

	void turnAbs(const glm::vec3& axis, float angle)
	{
		glm::mat3 mat{ glm::rotate(glm::mat4(1.0f), angle, axis) };
		right = mat * right;
		up = mat * up;
		back = mat * back;
	}




	void translateAbs(const glm::vec3& vec)
	{
		glm::mat4 mat{ glm::translate(glm::mat4(1.0f), vec) };
		pos = glm::vec3(mat * glm::vec4(pos, 1.0f));
	}
	void translateRel(const glm::vec3& vec)
	{
		glm::mat4 mat{ glm::translate(glm::mat4(1.0f), glm::vec3(vec.x * right + vec.y * up + vec.z * back)) };
		pos = glm::vec3(mat * glm::vec4(pos, 1.0f));
	}
	void repositionAbs(const glm::vec3& newPos)
	{
		pos = newPos;
	}

	glm::vec3 getBackDir()
	{
		return back;
	}

	glm::mat4 getViewMatrix()
	{
		glm::vec3 tempBack{ back };
		glm::vec3 tempRight{ right };
		
		float angle1{ (tempBack.x > 0 ? 1 : -1) * glm::acos(tempBack.z / glm::sqrt(tempBack.x * tempBack.x + tempBack.z * tempBack.z)) };
		glm::mat4 rot1{ glm::rotate(glm::mat4(1.0f), -angle1, glm::vec3(0.0f, 1.0f, 0.0f)) };
		tempBack = glm::mat3(rot1) * tempBack;
		tempRight = glm::mat3(rot1) * tempRight;
		float angle2{ (tempBack.y > 0 ? 1 : -1) * glm::acos(tempBack.z / glm::sqrt(tempBack.y * tempBack.y + tempBack.z * tempBack.z)) };
		glm::mat4 rot2{ glm::rotate(glm::mat4(1.0f), angle2, glm::vec3(1.0f, 0.0f, 0.0f)) };
		tempRight = glm::mat3(rot2) * tempRight;
		float angle3{ (tempRight.y > 0 ? 1 : -1) * glm::acos(tempRight.x / glm::sqrt(tempRight.x * tempRight.x + tempRight.y * tempRight.y)) };
		glm::mat4 rot3{ glm::rotate(glm::mat4(1.0f), -angle3, glm::vec3(0.0f, 0.0f, 1.0f)) };
		
		glm::mat4 mat{ rot3 * rot2 * rot1 * glm::translate(glm::mat4(1.0f), -glm::vec3(pos)) };
		return mat;
	}
};

#endif
