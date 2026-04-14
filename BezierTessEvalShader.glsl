#version 400 core

/** Isolines tartomány: a görbe t=0-tól t=1-ig halad egyenletes lépésközzel. */
/** Isolines domain: the curve goes from t=0 to t=1 with equal spacing. */
layout(isolines, equal_spacing) in;

/** Maximális kontrollpont szám a de Casteljau tömbhöz. */
/** Maximum control point count for the de Casteljau array. */
#define MAX_CONTROL_POINTS 16

/** Vetítési és nézeti mátrixok C++-ból. */
/** Projection and view matrices from C++. */
uniform mat4 matProjection;
uniform mat4 matModelView;

/** A tényleges kontrollpontok száma (a TCS output count 16, ezért uniform szükséges). */
/** The actual control point count (TCS output is always 16, so a uniform is needed). */
uniform int controlPointCount;

void main() {
	/** A t paraméter értéke 0.0 és 1.0 között: a görbe aktuális pozíciója. */
	/** The t parameter value between 0.0 and 1.0: the current position on the curve. */
	float t = gl_TessCoord.x;

	/** A görbe fokszáma (degree = pontok száma - 1). */
	/** Degree of the curve (degree = point count - 1). */
	int n = controlPointCount - 1;

	/** De Casteljau algoritmus: iteratív lineáris interpoláció a Bézier görbe kiértékeléséhez. */
	/** De Casteljau algorithm: iterative linear interpolation for Bezier curve evaluation. */
	/** Helyi másolat a kontrollpontokról, hogy az algoritmus módosíthassa őket. */
	/** Local copy of the control points so the algorithm can modify them. */
	vec4 points[MAX_CONTROL_POINTS];
	for (int i = 0; i < controlPointCount; i++)
		points[i] = gl_in[i].gl_Position;

	/** r-edik lépésnél minden szomszédos pár interpolálódik t arányban. */
	/** At step r, every adjacent pair is interpolated by ratio t. */
	/** mix(a, b, t) = a*(1-t) + b*t  (GLSL beépített függvény / built-in function). */
	for (int r = 1; r <= n; r++)
		for (int i = 0; i <= n - r; i++)
			points[i] = mix(points[i], points[i + 1], t);

	/** A görbe pontjának végleges pozíciója a vetítési és nézeti mátrixok alkalmazásával. */
	/** Final position of the curve point with projection and view matrices applied. */
	gl_Position = matProjection * matModelView * points[0];
}
