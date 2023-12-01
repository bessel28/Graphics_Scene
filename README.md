# Dynamic Scene Rendering using C++ and OpenGL

This project uses OpenGL to render 3 types of object.
1) Hand made object, hardcoded into the project
2) Object loaded from a .obj file
3) Procedurally generated Object

It includes custom lighting, shadows and shaders
![Alt text](Coursework3/Coursework3/Assets/image.png)
![Alt text](Coursework3/Coursework3/Assets/image2.png)

## Features

### Models
| Model | Model Type | Location |
|-------|------------|----------|
|Tank   | Hardcoded | [tankVerts.cpp](Coursework3\Coursework3\tankVerts.cpp)|
|Chest | Hardcoded | [Coursework3.cpp:4014](Coursework3\Coursework3\tankVerts.cpp)|
|Chest Lid | Hardcoded/Procedurally generated | [Coursework3.cpp:4001](Coursework3\Coursework3\tankVerts.cpp)|
|Sphere | Procedurally generated | [Coursework3.cpp:224](Coursework3\Coursework3\tankVerts.cpp)|
|Cylinder | Procedurally generated | [Coursework3.cpp:337](Coursework3\Coursework3\tankVerts.cpp)|
|Cannon | Loaded from .OBJ | [objParser.h](include\objParser.h)|
|Street Lamp | Loaded from .OBJ | [objParser.h](include\objParser.h)|

### Transformations
Most models underwent some level of transformation ranging from basic transforms, such as the road being rotated, translated and scaled into place, to more advanced transforms, such as the tank's gun being rotated around the tank's axis as the tank is driven around the scene.

### Cameras

The scene has 2 cameras available to use, which can be switched using the `tab` key. 

The model view camera, rotates around the center, always pointing at the middle target.

The fly through camera can be flown around the map and rotated to look anywhere using the mouse

### Lighting

3 lighting methods were implemented in the scene. Lighting method can be changed using the `alt` key.

The first light is a position light source, that comes from the sun as it moves around the scene.

The second light is a spotlight source that limits the light to a spotlight around the centre piece. Spotlights are also used for both street lights.

The third light is a directional light, which involves removing the sky and casting parallel lines from the direction of the sun

### Shadows

Shadows were created using a separate shadow map for each light source, using phong shading and percentage-close filtering to provide a smooth transition from shadow to light.

## Requirements

### GLFW
This project requires you to download the GLFW pre-compiled binary library at https://www.glfw.org/download.

You also need the header files `glfw3.h` and `glfw3native.h` found at https://github.com/glfw/glfw/tree/master/include/GLFW.

### GLM

GLM header files are needed and can be found at https://github.com/g-truc/glm/releases/.

### GL3W

GL3W is an OpenGL profile loader. The python script at https://github.com/skaslev/gl3w will generate the necessary files.