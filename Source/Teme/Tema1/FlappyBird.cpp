#include "FlappyBird.h"

#include <Core/Engine.h>
#include "Transform2D.h"


FlappyBird::FlappyBird() :
	fallAngleSpeed(100.f),
	riseAngleSpeed(200.f),
	gravity(1200.f),
	liftForce(300.f),
	numObstacles(4),
	obstacleWidth(80.f),
	obstacleDistance(325.f),
	obstacleStart(630.f),
	scaleSpeed(300.f),
	wingScaleSpeeddAcc(.3f),
	backgroundSpeed(50.f),
	shadersLoc("Source/Teme/Tema1/Shaders/"),
	texturesLoc("Source/Teme/Tema1/Textures/")
{
}

FlappyBird::~FlappyBird()
{
	delete bird;

	delete background;
	delete obstacle;
}

GLvoid FlappyBird::Init()
{
	srand((unsigned int)time(NULL));

	glm::ivec2 resolution = window->GetResolution();

	auto camera = GetSceneCamera();
	camera->SetOrthographic(
		0,
		(GLfloat)resolution.x,
		0,
		(GLfloat)resolution.y,
		0.01f,
		400);
	camera->SetPosition(glm::vec3(0, 0, 50));
	camera->SetRotation(glm::vec3(0, 0, 0));
	camera->Update();
	GetCameraInput()->SetActive(false);

	bird				= new Bird();
	trueScore			= 0.f;
	shownScore			= 0;
	
	sign				= 1.f;
	angle				= 0.f;
	fall				= true;
	collision			= false;
	renderHitBox		= false;
	canRender			= false;
	finalScore			= false;
	sound				= true;

	birdWingScaleSpeed	= 5.f;
	birdWingScale		= 1.f;
	centreX				= 200.f;
	centreY				= (GLfloat)resolution.y / 2.f;

	obstacleSpeed		= 300.f;
	speed				= liftForce;

	backgroundAngle		= 0.f;

	birdHitBoxRadius	= bird->GetHitBoxRadius();

	obstacle = new TexturedRectangle(
		obstacleWidth,
		(GLfloat)resolution.y,
		"obstacle",
		(texturesLoc + "bricks.jpg").c_str());
	background	= new TexturedRectangle(
		(GLfloat)resolution.x * 2.5f,
		(GLfloat)resolution.y * 2.5f,
		"background",
		(texturesLoc + "hole.jpg").c_str());

	GLfloat scaleFactor;
	GLfloat posX = obstacleStart;

	obstacles.resize(numObstacles);

	for (GLushort i = 0; i < numObstacles; ++i, posX += obstacleDistance)
	{
		scaleFactor			= 10.f / (rand() % 30 + 20);
		
		obstacles[i].posX	= posX;
		obstacles[i].scale	= scaleFactor;

		if (!(rand() % 5))
		{
			obstacles[i].isVariable = true;
			obstacles[i].scaleAngle = 0.f;
		}
		else
		{
			obstacles[i].isVariable = false;
			obstacles[i].scaleAngle = 90.f;
		}
	}

	{
		Shader* shader = new Shader("ObjectShader");
		shader->AddShader(shadersLoc + "VertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader(shadersLoc + "FragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

GLvoid FlappyBird::FrameStart()
{
	/* Clear the color buffer (using the previously set color) and depth buffer */
	glClearColor(0.5725f, 0.718f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Set the screen area where to draw */
	glm::ivec2 resolution = window->GetResolution();
	glViewport(0, 0, resolution.x, resolution.y);
}

GLvoid FlappyBird::Update(GLfloat deltaTimeSeconds)
{
	/* Don't render the first frame */
	if (!canRender)
	{
		canRender = true;
		return;
	}

	if (!collision)
	{
		/* Transformations that will be applied to each part of the bird */
		CalculateBirdAngle(deltaTimeSeconds);
		CalculateBirdMovement(deltaTimeSeconds);

		birdHitBox = bird->GetHitBox();
		CalculateBirdHitBox();

		/* Print the score */
		trueScore += deltaTimeSeconds * 2.f * obstacleSpeed / 200.f;
		obstacleSpeed += deltaTimeSeconds * 30.f;

		if ((GLint)trueScore > shownScore)
		{
			shownScore = (GLint)trueScore;
			std::cout << "SCORE: " << shownScore << "\n\n";
		}
	} else if (!finalScore)
	{
		finalScore = true;
		
		std::cout << "\n====================== FINAL SCORE: "
			<< (GLint)trueScore
			<< " =======================\n\n";
	}

	if (!collision)
	{
		collision = CheckBirdInMap();
	}

	/* Render the obstacles */
	RenderObstacles(deltaTimeSeconds);

	/* Render all the bird parts one by one */
	RenderBird(deltaTimeSeconds);

	/* Render the background */
	RenderBackground(deltaTimeSeconds);
}

GLvoid FlappyBird::FrameEnd()
{
}

GLvoid FlappyBird::OnKeyPress(GLint key, GLint mods)
{
	/* Fly, bird, fly */
	if (key == GLFW_KEY_SPACE && !collision)
	{
		if (sound)
		{
			PlaySound(TEXT("crow_caw.wav"), NULL, SND_FILENAME | SND_ASYNC);
		}
	
		fall = false;
		speed = liftForce;
	}

	/* Choose whether to render the hitbox or not */
	if (key == GLFW_KEY_H)
	{
		renderHitBox = !renderHitBox;
	}

	/* Toggle the bird's sound on/off */
	if (key == GLFW_KEY_S)
	{
		sound = !sound;
	}
}

GLvoid FlappyBird::CalculateBirdAngle(GLfloat deltaTimeSeconds)
{
	if (fall)
	{
		angle -= deltaTimeSeconds * fallAngleSpeed;

		if (angle <= -90.f)
		{
			angle = -90.f;
		}
	}
	else
	{
		angle += deltaTimeSeconds * riseAngleSpeed;

		if (angle >= 30.f)
		{
			fall = true;
		}
	}
}

GLvoid FlappyBird::CalculateBirdMovement(GLfloat deltaTimeSeconds)
{
	speed -= gravity * deltaTimeSeconds;

	/* The equation of motion */
	centreY +=
		speed * deltaTimeSeconds
		+ gravity * deltaTimeSeconds * deltaTimeSeconds / 2.f;

	/* Change the scale of the wing */
	birdWingScaleSpeed += deltaTimeSeconds * wingScaleSpeeddAcc;
	birdWingScale -= deltaTimeSeconds * birdWingScaleSpeed * sign;

	if (birdWingScale >= 1.f)
	{
		sign = 1.f;
	}
	else if (birdWingScale <= -1.f)
	{
		sign = -1.f;
	}
}

GLvoid FlappyBird::CalculateBirdHitBox()
{
	glm::vec3 hitBoxPoint(0.f, 0.f, 1.f);

	glm::mat3 hitBoxMatrix = Transform2D::Translate(centreX, centreY);
	hitBoxMatrix *= Transform2D::Rotate(RADIANS(angle));

	for (auto& point : birdHitBox)
	{
		/* Rotate the bird's hitbox */
		hitBoxPoint[0]	= point.coordX;
		hitBoxPoint[1]	= point.coordY;
		hitBoxPoint		= hitBoxMatrix * hitBoxPoint;

		point.coordX	= hitBoxPoint[0];
		point.coordY	= hitBoxPoint[1];
	}
}

GLvoid FlappyBird::RenderBird(GLfloat deltaTimeSeconds)
{
	GLfloat offsetX = 0.f, offsetY = 0.f;

	/* Render the hit box */
	if (renderHitBox)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		RenderBodyPart(
			bird->GetHitBoxMesh(offsetX, offsetY),
			offsetX,
			offsetY,
			deltaTimeSeconds);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	/* Render the wing */
	RenderBodyPart(
		bird->GetWingMesh(offsetX, offsetY),
		offsetX,
		offsetY,
		deltaTimeSeconds);

	/* Render the beak */
	RenderBodyPart(
		bird->GetBeakMesh(offsetX, offsetY),
		offsetX,
		offsetY,
		deltaTimeSeconds);

	/* Render the eye */
	RenderBodyPart(
		bird->GetEyeMesh(offsetX, offsetY),
		offsetX,
		offsetY,
		deltaTimeSeconds);

	/* Render the head */
	RenderBodyPart(
		bird->GetHeadMesh(offsetX, offsetY),
		offsetX,
		offsetY,
		deltaTimeSeconds);

	/* Render the body */
	RenderBodyPart(
		bird->GetBodyMesh(offsetX, offsetY),
		offsetX,
		offsetY,
		deltaTimeSeconds);

	/* Render the tail */
	RenderBodyPart(
		bird->GetTailMesh(offsetX, offsetY),
		offsetX,
		offsetY,
		deltaTimeSeconds);
}

GLvoid FlappyBird::RenderBodyPart(
	Mesh* bodyPart,
	GLfloat offsetX,
	GLfloat offsetY,
	GLfloat deltaTimeSeconds)
{
	modelMatrix = Transform2D::Translate(centreX, centreY);
	modelMatrix *= Transform2D::Rotate(RADIANS(angle));
	modelMatrix *= Transform2D::Translate(offsetX, offsetY);

	if (!strcmp(bodyPart->GetMeshID(), "wing"))
	{
		modelMatrix *= Transform2D::Scale(1.f, birdWingScale);
	}

	RenderMesh2D(bodyPart, shaders["VertexColor"], modelMatrix);
}

inline GLboolean FlappyBird::IsObstacleInMap(Obstacle& obs)
{
	return obs.posX + obstacleWidth >= 0.f;
}

GLvoid FlappyBird::RenderObstacles(GLfloat deltaTimeSeconds)
{
	glm::ivec2 resolution = window->GetResolution();
	GLfloat scaleFactor;

	for (Obstacle& obs : obstacles)
	{
		if (!IsObstacleInMap(obs))
		{
			scaleFactor		= 10.f / (rand() % 31 + 20);

			obs.posX		= (GLfloat)resolution.x;
			obs.scale		= scaleFactor;

			/* one fifth of obstacles have variable scaling */
			if (!(rand() % 5))
			{
				obs.isVariable = true;
				obs.scaleAngle = 0.f;
			} else
			{
				obs.isVariable = false;
				obs.scaleAngle = 90.f;
			}
		} else
		{
			/* Render the lower obstacle */
			modelMatrix = Transform2D::Translate(obs.posX, 0.f);
			modelMatrix *= Transform2D::Scale(
				1.f,
				obs.scale * sin(RADIANS(obs.scaleAngle)));
			RenderTexturedMesh(
				obstacle->GetMesh(),
				shaders["ObjectShader"],
				modelMatrix,
				obstacle->GetTexture()
			);
			
			/* Render the upper obstacle */
			modelMatrix = Transform2D::Translate(
				obs.posX + obstacleWidth,
				(GLfloat)resolution.y);
			modelMatrix *= Transform2D::Rotate(RADIANS(180.f));
			modelMatrix *= Transform2D::Scale(
				1.f,
				(0.6f - obs.scale) * sin(RADIANS(obs.scaleAngle)));
			RenderTexturedMesh(
				obstacle->GetMesh(),
				shaders["ObjectShader"],
				modelMatrix,
				obstacle->GetTexture()
			);

			/* Only for the obstacle below the bird, check for collisions*/
			if (centreX + birdHitBoxRadius >= obs.posX
				&& centreX - birdHitBoxRadius <= obs.posX + obstacleWidth)
			{
				if (!collision)
				{
					/* Check for collision with the lower obstacle */
					collision = CheckBirdCollision(
						obs.posX,
						0.f,
						resolution.y * obs.scale * sin(RADIANS(obs.scaleAngle)));
				}
				if (!collision)
				{
					/* Check for collision with the upper obstacle */
					collision = CheckBirdCollision(
						obs.posX,
						resolution.y * (1.f - (0.6f - obs.scale)
							* sin(RADIANS(obs.scaleAngle))),
						(GLfloat)resolution.y);
				}
			}			
		}

		/* Freeze the scene when a collision happens */
		if (!collision)
		{
			obs.posX -= deltaTimeSeconds * obstacleSpeed;

			/* Scale the obstacle if required*/
			if (obs.isVariable)
			{
				obs.scaleAngle += deltaTimeSeconds * scaleSpeed;

				if (obs.scaleAngle >= 180.f)
				{
					obs.scaleAngle = 0.f;
				}
			}
		}
	}
}

GLvoid FlappyBird::RenderBackground(GLfloat deltaTimeSeconds)
{
	glm::ivec2 resolution = window->GetResolution();

	backgroundAngle += backgroundSpeed * deltaTimeSeconds;

	if (backgroundAngle >= 360.f)
	{
		backgroundAngle = 0.f;
	}

	/* Rotate the background around its centre */
	modelMatrix = Transform2D::Translate(
		(GLfloat)resolution.x / 2.f,
		(GLfloat)resolution.y / 2.f);
	modelMatrix *= Transform2D::Rotate(RADIANS(backgroundAngle));
	modelMatrix *= Transform2D::Translate(
		-(GLfloat)resolution.x * 1.25f,
		-(GLfloat)resolution.y * 1.25f);

	RenderTexturedMesh(
		background->GetMesh(),
		shaders["ObjectShader"],
		modelMatrix,
		background->GetTexture()
	);
}

GLvoid FlappyBird::RenderTexturedMesh(
	Mesh* mesh,
	Shader* shader,
	const glm::mat3& modelMatrix,
	Texture2D* texture)
{
	if (!mesh || !shader || !shader->GetProgramID())
	{
		return;
	}

	glm::mat3 mm = modelMatrix;
	glm::mat4 model = glm::mat4(
		mm[0][0],	mm[0][1],	mm[0][2],	0.f,
		mm[1][0],	mm[1][1],	mm[1][2],	0.f,
		0.f,		0.f,		mm[2][2],	0.f,
		mm[2][0],	mm[2][1],	0.f,		1.f);

	// render an object using the specified shader and the specified position
	glUseProgram(shader->program);

	// Bind model matrix
	GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(model));

	// Bind view matrix
	glm::mat4 viewMatrix = GetSceneCamera()->GetViewMatrix();
	int loc_view_matrix = glGetUniformLocation(shader->program, "View");
	glUniformMatrix4fv(
		loc_view_matrix,
		1,
		GL_FALSE,
		glm::value_ptr(viewMatrix));

	// Bind projection matrix
	glm::mat4 projectionMatrix = GetSceneCamera()->GetProjectionMatrix();
	int loc_projection_matrix = glGetUniformLocation(
		shader->program,
		"Projection");
	glUniformMatrix4fv(
		loc_projection_matrix,
		1,
		GL_FALSE,
		glm::value_ptr(projectionMatrix));

	// Activate texture location 0
	glActiveTexture(GL_TEXTURE0);

	// Bind the texture1 ID
	glBindTexture(GL_TEXTURE_2D, texture->GetTextureID());

	// Send texture uniform value
	glUniform1i(glGetUniformLocation(shader->program, "texture"), 0);

	// Draw the object
	glBindVertexArray(mesh->GetBuffers()->VAO);
	glDrawElements(
		mesh->GetDrawMode(),
		static_cast<int>(mesh->indices.size()),
		GL_UNSIGNED_SHORT,
		0);
}

GLboolean FlappyBird::CheckBirdInMap()
{
	glm::ivec2 resolution = window->GetResolution();

	for (auto& point : birdHitBox)
	{
		if (point.coordY >= resolution.y || point.coordY <= 0)
		{
			std::cout << point.coordY << '\n';
			return true;
		}
	}
	
	return false;
}

GLboolean FlappyBird::CheckBirdCollision(
	GLfloat lowX,
	GLfloat lowY,
	GLfloat highY)
{
	GLfloat highX = lowX + obstacleWidth;

	for (auto& point : birdHitBox)
	{
		/* Check  if the current point of the hit box collides with the given */
		/* obstacle */
		if (point.coordX <= highX && point.coordX >= lowX
			&& point.coordY <= highY && point.coordY >= lowY)
		{
			return true;
		}
	}

	return false;
}