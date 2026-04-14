// Haruna_Muhammad_Idris_SF704D_2nd_Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>




enum eVertexArrayObject {
	VAOBezierData,
	VAOCount
};
enum eBufferObject {
	VBOControlPoints,
	BOCount
};
enum eProgram {
	BezierCurveProgram,
	ControlPointProgram,
	ProgramCount
};
enum eTexture {
	NoTexture,		// fixes 0 sized array problem
	TextureCount
};

#include "common.cpp"

/** Maximum kontrollpont szám. Meg kell egyeznie a tessellation shaderekben definiált értékkel! */
/** Maximum control point count. Must match the value defined in the tessellation shaders! */
#define MAX_CONTROL_POINTS 16

GLchar	windowTitle[] = "Bezier Curve Editor";
GLfloat	aspectRatio;

/** -1 jelenti, hogy nem vonszolunk semmit. */
/** -1 means we are not dragging anything. */
GLint	dragged = -1;

/** A Bézier görbe kontrollpontjai. Kezdetben 4 pont (köbös görbe). */
/** Control points of the Bezier curve. Initially 4 points (cubic curve). */
static vector<vec2> controlPoints = {
	vec2(-0.5f, -0.3f),
	vec2(-0.2f,  0.4f),
	vec2(0.2f, -0.4f),
	vec2(0.5f,  0.3f)
};

/** Shader uniform változók location-jei a Bézier tessellation programhoz. */
/** Shader uniform variable locations for the Bezier tessellation program. */
GLuint	locationBezierMatProjection, locationBezierMatModelView;
GLuint	locationBezierControlPointCount, locationBezierTessLevel;

/** Shader uniform változók location-jei a kontrollpont megjelenítő programhoz. */
/** Shader uniform variable locations for the control point display program. */
GLuint	locationCtrlMatProjection, locationCtrlMatModelView, locationCtrlColor;

/** Feltölti a kontrollpontokat a GPU memóriájába. Minden módosítás után hívni kell. */
/** Uploads the control points to GPU memory. Must be called after every modification. */
void uploadControlPoints() {
	glBindBuffer(GL_ARRAY_BUFFER, BO[VBOControlPoints]);
	/** GL_DYNAMIC_DRAW: az adat gyakran változik (vonszolás, hozzáadás, törlés). */
	/** GL_DYNAMIC_DRAW: the data changes frequently (dragging, adding, removing). */
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(vec2), controlPoints.data(), GL_DYNAMIC_DRAW);
}

/** Visszaadja két pont távolságának négyzetét. A négyzetgyökvonás megtakarítható összehasonlításnál. */
/** Returns the square of the distance between two points. Saves a sqrt for comparisons. */
GLfloat distanceSquare(vec2 p1, vec2 p2) {
	vec2	delta = p1 - p2;
	return dot(delta, delta);		// delta.x * delta.x + delta.y * delta.y
}

/** Visszaadja az egérhez legközelebbi kontrollpont indexét az érzékenységen belül, vagy -1-et. */
/** Returns the index of the control point closest to the mouse within sensitivity, or -1. */
GLint getActivePoint(GLfloat sensitivity, vec2 mousePosition) {
	GLfloat	sensitivitySquare = sensitivity * sensitivity;
	for (GLint i = 0; i < (GLint)controlPoints.size(); i++)
		if (distanceSquare(controlPoints[i], mousePosition) < sensitivitySquare)
			return i;
	return -1;
}

/** Visszaadja az egérhez abszolút legközelebbi kontrollpont indexét (érzékenység nélkül). */
/** Returns the index of the absolutely nearest control point to the mouse (without sensitivity). */
GLint getNearestPoint(vec2 mousePosition) {
	GLint	nearest = 0;
	GLfloat	minDist = distanceSquare(controlPoints[0], mousePosition);
	for (GLint i = 1; i < (GLint)controlPoints.size(); i++) {
		GLfloat d = distanceSquare(controlPoints[i], mousePosition);
		if (d < minDist) { minDist = d; nearest = i; }
	}
	return nearest;
}

/** Konvertálja a pixelkoordinátákat világkoordinátákká az ortografikus vetítés figyelembevételével. */
/** Converts pixel coordinates to world coordinates considering the orthographic projection. */
vec2 pixelToWorld(double xPos, double yPos) {
	dvec2	mousePosition;
	/** Normalizált eszközkoordináták: [-1, +1] tartomány. */
	/** Normalized device coordinates: [-1, +1] range. */
	mousePosition.x = xPos * 2.0f / (GLdouble)windowWidth - 1.0f;
	mousePosition.y = ((GLdouble)windowHeight - yPos) * 2.0f / (GLdouble)windowHeight - 1.0f;
	/** Aspect ratio korrekció az ortografikus vetítéshez. */
	/** Aspect ratio correction for the orthographic projection. */
	if (windowWidth < windowHeight)
		mousePosition.y /= aspectRatio;
	else
		mousePosition.x *= aspectRatio;
	return vec2(mousePosition);
}

/** Inicializálja a Bézier tessellation shader programot és lekérdezi a uniform location-öket. */
/** Initializes the Bezier tessellation shader program and queries the uniform locations. */
void initBezierTessProgram() {
	ShaderInfo bezierShaders[] = {
		{ GL_VERTEX_SHADER,          "./BezierVertShader.glsl"     },
		{ GL_TESS_CONTROL_SHADER,    "./BezierTessContShader.glsl" },
		{ GL_TESS_EVALUATION_SHADER, "./BezierTessEvalShader.glsl" },
		{ GL_FRAGMENT_SHADER,        "./BezierFragShader.glsl"     },
		{ GL_NONE, nullptr }
	};
	/** A Bézier görbe shader programjának elkészítése. */
	/** Creating the Bezier curve shader program. */
	program[BezierCurveProgram] = LoadShaders(bezierShaders);
	/** Shader változók location lekérdezése. */
	/** Getting shader variable locations. */
	locationBezierMatProjection = glGetUniformLocation(program[BezierCurveProgram], "matProjection");
	locationBezierMatModelView = glGetUniformLocation(program[BezierCurveProgram], "matModelView");
	locationBezierControlPointCount = glGetUniformLocation(program[BezierCurveProgram], "controlPointCount");
	locationBezierTessLevel = glGetUniformLocation(program[BezierCurveProgram], "tessLevel");
}

/** Inicializálja a kontrollpont megjelenítő shader programot és lekérdezi a uniform location-öket. */
/** Initializes the control point display shader program and queries the uniform locations. */
void initControlPointProgram() {
	ShaderInfo ctrlShaders[] = {
		{ GL_VERTEX_SHADER,   "./ControlPointVertShader.glsl" },
		{ GL_FRAGMENT_SHADER, "./ControlPointFragShader.glsl" },
		{ GL_NONE, nullptr }
	};
	/** A kontrollpont megjelenítő shader program elkészítése. */
	/** Creating the control point display shader program. */
	program[ControlPointProgram] = LoadShaders(ctrlShaders);
	/** Shader változók location lekérdezése. */
	/** Getting shader variable locations. */
	locationCtrlMatProjection = glGetUniformLocation(program[ControlPointProgram], "matProjection");
	locationCtrlMatModelView = glGetUniformLocation(program[ControlPointProgram], "matModelView");
	locationCtrlColor = glGetUniformLocation(program[ControlPointProgram], "color");
}

void initShaderProgram() {
	initBezierTessProgram();
	initControlPointProgram();

	/** Csatoljuk a vertex array objektumot és beállítjuk a VBO-t. */
	/** Attach the vertex array object and set up the VBO. */
	glBindVertexArray(VAO[VAOBezierData]);
	glBindBuffer(GL_ARRAY_BUFFER, BO[VBOControlPoints]);

	/** Ezen adatok szolgálják a location = 0 vertex attribútumot (pozíció). */
	/** These values are for location = 0 vertex attribute (position). */
	/** 2D pozíció (vec2), tightly packed, nincs offset. */
	/** 2D position (vec2), tightly packed, no offset. */
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
	glEnableVertexAttribArray(0);

	/** A kezdeti kontrollpontok feltöltése a GPU-ra. */
	/** Upload the initial control points to the GPU. */
	uploadControlPoints();

	/** Mátrixok kezdőértékének beállítása. */
	/** Set the matrices to the initial values. */
	matModel = mat4(1.0);
	matView = lookAt(
		vec3(0.0f, 0.0f, 9.0f),	// the position of your camera, in world space
		vec3(0.0f, 0.0f, 0.0f),	// where you want to look at, in world space
		vec3(0.0f, 1.0f, 0.0f));	// upVector

	/** A kontrollpontok mérete pixelben. */
	/** Control point size in pixels. */
	glPointSize(10.0f);
}

void display(GLFWwindow* window, double currentTime) {
	/** Töröljük le a kiválasztott buffereket! */
	/** Let's clear the selected buffers! */
	glClear(GL_COLOR_BUFFER_BIT);

	int n = (int)controlPoints.size();
	/** Legalább 2 kontrollpont szükséges bármilyen görbe rajzolásához. */
	/** At least 2 control points are needed to draw any curve. */
	if (n < 2) return;

	glBindVertexArray(VAO[VAOBezierData]);

	/** --- Kék kontrollpolygon rajzolása (összekötjük a szomszédos kontrollpontokat) --- */
	/** --- Draw the blue control polygon (connecting adjacent control points) --- */
	glUseProgram(program[ControlPointProgram]);
	glUniform3f(locationCtrlColor, 0.0f, 0.0f, 1.0f);		// kék / blue
	glDrawArrays(GL_LINE_STRIP, 0, n);

	/** --- Zöld Bézier görbe rajzolása tessellation shaderekkel --- */
	/** --- Draw the green Bezier curve using tessellation shaders --- */
	/** A görbe fokszáma = n - 1, ahol n a kontrollpontok száma. */
	/** Degree of curve = n - 1, where n is the number of control points. */
	glUseProgram(program[BezierCurveProgram]);
	glUniform1i(locationBezierControlPointCount, n);
	/** 200 szegmens elegendő a folyamatos megjelenéshez. */
	/** 200 segments is sufficient for a smooth appearance. */
	glUniform1f(locationBezierTessLevel, 200.0f);
	/** GL_PATCH_VERTICES-t be kell állítani az aktuális kontrollpont számra. */
	/** GL_PATCH_VERTICES must be set to the current control point count. */
	glPatchParameteri(GL_PATCH_VERTICES, n);
	glDrawArrays(GL_PATCHES, 0, n);

	/** --- Piros kontrollpontok rajzolása (a görbe felett, hogy láthatók legyenek) --- */
	/** --- Draw the red control points (on top of the curve so they are visible) --- */
	glUseProgram(program[ControlPointProgram]);
	glUniform3f(locationCtrlColor, 1.0f, 0.0f, 0.0f);		// piros / red
	glDrawArrays(GL_POINTS, 0, n);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	/** A minimalizálás nem fog fagyni a minimum 1 értékkel. */
	/** Minimize will not freeze with minimum value 1. */
	windowWidth = glm::max(width, 1);
	windowHeight = glm::max(height, 1);
	aspectRatio = (float)windowWidth / (float)windowHeight;

	/** A kezelt képernyő beállítása a teljes (0, 0, width, height) területre. */
	/** Set the viewport for the full (0, 0, width, height) area. */
	glViewport(0, 0, windowWidth, windowHeight);

	/** Orthographic projekció beállítása, worldSize lesz a szélesség és magasság közül a kisebbik. */
	/** Set up orthographic projection, worldSize will equal the smaller value of width or height. */
	if (windowWidth < windowHeight)
		matProjection = ortho(-worldSize, worldSize, -worldSize / aspectRatio, worldSize / aspectRatio, -100.0, 100.0);
	else
		matProjection = ortho(-worldSize * aspectRatio, worldSize * aspectRatio, -worldSize, worldSize, -100.0, 100.0);

	matModelView = matView * matModel;

	/** Uniform változók beállítása a Bézier tessellation programhoz. */
	/** Setup uniform variables for the Bezier tessellation program. */
	glUseProgram(program[BezierCurveProgram]);
	glUniformMatrix4fv(locationBezierMatProjection, 1, GL_FALSE, value_ptr(matProjection));
	glUniformMatrix4fv(locationBezierMatModelView, 1, GL_FALSE, value_ptr(matModelView));

	/** Uniform változók beállítása a kontrollpont megjelenítő programhoz. */
	/** Setup uniform variables for the control point display program. */
	glUseProgram(program[ControlPointProgram]);
	glUniformMatrix4fv(locationCtrlMatProjection, 1, GL_FALSE, value_ptr(matProjection));
	glUniformMatrix4fv(locationCtrlMatModelView, 1, GL_FALSE, value_ptr(matModelView));
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	/** ESC billentyűre kilépés. */
	/** Exit on ESC key. */
	if ((action == GLFW_PRESS) && (key == GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, GLFW_TRUE);
	/** A billentyűk lenyomásának és felengedésének regisztrálása. */
	/** Register key press and release events. */
	if (action == GLFW_PRESS)
		keyboard[key] = GL_TRUE;
	else if (action == GLFW_RELEASE)
		keyboard[key] = GL_FALSE;
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
	/** Ha vonszolunk egy kontrollpontot, frissítjük a pozícióját és újratöltjük a GPU-ra. */
	/** If we are dragging a control point, update its position and re-upload to the GPU. */
	if (dragged >= 0) {
		controlPoints[dragged] = pixelToWorld(xPos, yPos);
		uploadControlPoints();
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	double	xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);
	vec2	mousePosition = pixelToWorld(xPos, yPos);

	/** Bal egérgomb lenyomása: vonszolás indítása, vagy új kontrollpont hozzáadása. */
	/** Left mouse button press: start dragging, or add a new control point. */
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		/** Megpróbálunk egy meglévő kontrollpontot megfogni (0.1 egységen belül). */
		/** Try to grab an existing control point (within 0.1 world units). */
		dragged = getActivePoint(0.1f, mousePosition);
		/** Ha nem sikerült fogni semmit, új pontot adunk hozzá. */
		/** If nothing was grabbed, add a new control point. */
		if (dragged == -1 && (int)controlPoints.size() < MAX_CONTROL_POINTS) {
			controlPoints.push_back(mousePosition);
			uploadControlPoints();
			cout << "Control point added. Total: " << controlPoints.size() << " (degree " << controlPoints.size() - 1 << ")" << endl;
		}
	}
	/** Bal egérgomb felengedése: vonszolás befejezése. */
	/** Left mouse button release: stop dragging. */
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		dragged = -1;

	/** Jobb egérgomb lenyomása: legközelebbi kontrollpont törlése. */
	/** Right mouse button press: remove the nearest control point. */
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		/** Legalább 2 kontrollpontot megtartunk (1 pontból nem rajzolható görbe). */
		/** Keep at least 2 control points (a curve cannot be drawn from 1 point). */
		if ((int)controlPoints.size() > 2) {
			GLint nearest = getNearestPoint(mousePosition);
			controlPoints.erase(controlPoints.begin() + nearest);
			uploadControlPoints();
			cout << "Control point removed. Total: " << controlPoints.size() << " (degree " << controlPoints.size() - 1 << ")" << endl;
		}
		else {
			cout << "Cannot remove: minimum 2 control points required." << endl;
		}
	}
}

int main(void) {
	/** Az alkalmazáshoz kapcsolódó előkészítő lépések. */
	/** The first initialization steps of the program. */
	/** OpenGL 4.0 szükséges a tessellation shaderekhez! */
	/** OpenGL 4.0 is required for tessellation shaders! */
	init(4, 0, GLFW_OPENGL_COMPAT_PROFILE);
	/** A shader programok betöltése. */
	/** Loading the shader programs for rendering. */
	initShaderProgram();
	/** A window elkészítése után a képernyő beállítása. */
	/** Setup screen after window creation. */
	framebufferSizeCallback(window, windowWidth, windowHeight);
	/** Karakterkódolás a szövegekhez. */
	/** Setting locale for characters of texts. */
	setlocale(LC_ALL, "");

	cout << "Bezier Curve Editor" << endl << endl;
	cout << "Keyboard control" << endl;
	cout << "ESC\t\t\texit" << endl << endl;
	cout << "Mouse control" << endl;
	cout << "Left click (empty)\tadd new control point (max " << MAX_CONTROL_POINTS << ")" << endl;
	cout << "Left click + drag\tmove control point" << endl;
	cout << "Right click\t\tremove nearest control point (min 2)" << endl << endl;
	cout << "Weekly tasks" << endl;
	cout << "Feladat 1: Zöld Bezier-gorbe rajzolasa 4 kontrollponttal." << endl;
	cout << "Feladat 2: 4 kontrollpont rajzolasa piros szinnel." << endl;
	cout << "Feladat 3: Kontrollpolygon rajzolasa kek szinnel." << endl;
	cout << "Feladat 4: Kontrollpontok mozgatasa drag-and-drop technikával." << endl;
	cout << "Feladat 5: Kontrollpontok hozzaadasa/torlese eger gombokkal." << endl;
	cout << "Task 1: Draw a green Bezier curve defined by 4 control points." << endl;
	cout << "Task 2: Draw the 4 control points with red color." << endl;
	cout << "Task 3: Draw the control polygon sides with blue color." << endl;
	cout << "Task 4: Move control points by drag and drop." << endl;
	cout << "Task 5: Add/remove control points with mouse buttons." << endl;

	/** A megadott window struktúra "close flag" vizsgálata. */
	/** Checks the "close flag" of the specified window. */
	while (!glfwWindowShouldClose(window)) {
		/** A kód, amellyel rajzolni tudunk a GLFWwindow objektumunkba. */
		/** Call display function which will draw into the GLFWwindow object. */
		display(window, glfwGetTime());
		/** Double buffered működés. */
		/** Double buffered working = swap the front and back buffer here. */
		glfwSwapBuffers(window);
		/** Események kezelése az ablakunkkal kapcsolatban. */
		/** Handle events related to our window. */
		glfwPollEvents();
	}
	/** Felesleges objektumok törlése. */
	/** Cleanup the unnecessary objects. */
	cleanUpScene(EXIT_SUCCESS);
	/** Kilépés EXIT_SUCCESS kóddal. */
	/** Stop the software and exit with EXIT_SUCCESS code. */
	return EXIT_SUCCESS;
}
