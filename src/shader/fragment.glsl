#version 400 core

in Attribute {
	vec3 position;
	vec3 normal;
} vertexIn;

uniform mat4 mv;

out vec3 color;

// Blinn-Phong shading
vec3 Lighting(vec3 light_position, float light_power, int light_n) {
	vec3 light_color = vec3(1.f, 1.f, 1.f);

	const vec3 diffuse_color  = vec3(0.9f, 0.3f, 0.3f);
	const vec3 ambient_color  = vec3(0.4f, 0.4f, 0.4f) * diffuse_color;
	const vec3 specular_color = vec3(0.3f, 0.3f, 0.3f);
	const float shininess = 16.0;

	vec3 light_direction = light_position - vertexIn.position;
	float distance = length(light_direction);
	distance *= distance;

	light_direction = normalize(light_direction);
	vec3 normal = normalize(vertexIn.normal);
	float cos_theta = clamp(dot(normal, light_direction), 0.f, 1.f);
	float lambertian = clamp(cos_theta, 0.f, 1.f);

	vec3 eye_direction = normalize(-vertexIn.position);
	vec3 half_direction = normalize(light_direction + eye_direction);
	float cos_alpha = dot(half_direction, normal);
	float specular = pow(clamp(cos_alpha, 0.f, 1.f), shininess);

	color =
		ambient_color / light_n +
		diffuse_color * lambertian * light_color * light_power / distance +
		specular_color * specular  * light_color * light_power / distance;

	return color;
}

void main() {
	vec3 light_position_0 = (mv * vec4( 20.f, 20.f, 10.f, 1.f)).xyz;
	vec3 light_position_1 = (mv * vec4(-20.f, 10.f,-10.f, 1.f)).xyz;

	vec3 c = vec3(0.f);

	c += Lighting(light_position_0, 40.f, 2);
	c += Lighting(light_position_1, 30.f, 2);
	// c = vec4(1.f, 1.f, 1.f, 1.f);
	// c = vertexIn.normal;

	color = c;
}
