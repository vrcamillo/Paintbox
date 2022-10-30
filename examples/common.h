#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "GLFW/glfw3.h"

static GLFWwindow* window;

void centralize_window(GLFWwindow* window) {
	auto monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
	
	int window_width, window_height;
	glfwGetWindowSize(window, &window_width, &window_height);
	
	int x = (vidmode->width - window_width) / 2;
	int y = (vidmode->height - window_height) / 2;
	glfwSetWindowPos(window, x, y);
}

bool init();
void do_frame();

int main() {
	if (!glfwInit()) return -1;
	
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	window = glfwCreateWindow(1920 * 3 / 4, 1080 * 3 / 4, "Paintbox", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	
	centralize_window(window);
	glfwMakeContextCurrent(window);
	
	init();
	
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		do_frame();
		
		glfwSwapBuffers(window);
	}
	
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}