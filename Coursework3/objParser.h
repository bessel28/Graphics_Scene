


void objParser(const char* filename, std::vector<std::vector<float>>& objects, bool readTex) {
	printf("Loading obj %s...\n", filename);
	char* obj = read_file(filename);
	const char* delim = "\n";
	char* saveptr, * saveptr2;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;

	std::vector<float > object;

	int objNum = 0;

	char* line = strtok_s(obj, delim, &saveptr);
	// Reads each line 1 by 1
	while (line != NULL)
	{
		std::string curline(line);
		char* words = strtok_s(line, " ", &saveptr2);
		std::string word(words);
		// If the first word is a comment
		if (word == "#") {
			words = strtok_s(NULL, " ", &saveptr2);
			std::string word(words);
			// If that comment defines a new object, save the object and start the next one
			if (word == "object") {
				if (objNum != 0) {
					objects.push_back(object);
					object.clear();
				}
				objNum++;
			}
		}
		// If the first word defines a new object, save the object and start the next one
		else if (word == "o") {
			if (objNum != 0) {
				objects.push_back(object);
				object.clear();
			}
			objNum++;
		}
		// Add Vertex
		else if (word == "v") {
			glm::vec3 v;
			words = strtok_s(NULL, " ", &saveptr2);
			v.x = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.y = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.z = std::strtof(words, nullptr);
			vertices.push_back(v);
		}
		// Add vertex Normal
		else if (word == "vn") {
			glm::vec3 v;
			words = strtok_s(NULL, " ", &saveptr2);
			v.x = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.y = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.z = std::strtof(words, nullptr);
			normals.push_back(v);
		}
		// Add vertex tex coords
		else if (word == "vt") {
			if (readTex) {
				glm::vec2 v;
				words = strtok_s(NULL, " ", &saveptr2);
				v.x = std::strtof(words, nullptr);
				words = strtok_s(NULL, " ", &saveptr2);
				v.y = std::strtof(words, nullptr);
				texCoords.push_back(v);
			}
		}
		// Add face to the return object
		else if (word == "f") {
			std::vector<std::vector<float>> face;
			// Loop over each word in a quad/triangle
			for (int i = 0; i < 4; i++) {
				std::vector<float> vertex;
				char* saveptr3;
				int index;
				words = strtok_s(NULL, " ", &saveptr2);
				// incase it's a triangle not a quad
				if (words != NULL) {
					index = std::atoi(strtok_s(words, "/", &saveptr3));
					glm::vec3 v = vertices.at(index - 1);
					vertex.push_back(v.x);
					vertex.push_back(v.y);
					vertex.push_back(v.z);

					index = std::atoi(strtok_s(NULL, "/", &saveptr3));
					if (readTex) {
						glm::vec2 t = texCoords.at(index - 1);
						vertex.push_back(t.x);
						vertex.push_back(t.y);
					}

					index = std::atoi(strtok_s(NULL, "/", &saveptr3));
					glm::vec3 n = normals.at(index - 1);
					vertex.push_back(n.x);
					vertex.push_back(n.y);
					vertex.push_back(n.z);

					face.push_back(vertex);
				}
			}
			int vertSize = readTex ? 8 : 6;
			// Add triangle to return object
			object.insert(object.end(), face.at(0).data(), face.at(0).data() + vertSize);
			object.insert(object.end(), face.at(1).data(), face.at(1).data() + vertSize);
			object.insert(object.end(), face.at(2).data(), face.at(2).data() + vertSize);
			// if quad add the second triangle too!
			if (face.size() == 4) {
				object.insert(object.end(), face.at(0).data(), face.at(0).data() + vertSize);
				object.insert(object.end(), face.at(2).data(), face.at(2).data() + vertSize);
				object.insert(object.end(), face.at(3).data(), face.at(3).data() + vertSize);
			}
		}
		line = strtok_s(NULL, delim, &saveptr);
	}
	// return object
	objects.push_back(object);
	printf("Loaded OBJ %s\n", filename);
	free(obj);
}