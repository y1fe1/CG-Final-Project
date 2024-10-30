#version 410
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;

//void main()
//{
//    // Simple Vague
//    vec3 color = vec3(0.0);
//    float offset = 1.0 / 300.0;
//
//    for (int x = -1; x <= 1; x++) {
//        for (int y = -1; y <= 1; y++) {
//            color += texture(scene, TexCoords + vec2(x, y) * offset).rgb;
//        }
//    }
//    color /= 9.0; // 取平均值
//    FragColor = vec4(color, 1.0);
//}
// 

//void main() {
//    FragColor = texture(scene, TexCoords);
//}


//  Show all red
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Output pure red
}
