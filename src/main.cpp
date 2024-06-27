//return code 0 - code executed correctly
//return code 1 - failure in window creation
//return code 2 - failure in GLAD initialization

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/stb_image.h>
#include <portaudio/portaudio.h>
#include <utf8cpp/utf8.h>

#include "Utilities.h"
#include "Shader.h"
#include "Cam.h"
#include "Synthesizer.h"
#include "Singer.h"
#include "VisualNoteDrawer.h"
#include "AmpDrawer.h"
#include "FreqDrawer.h"
#include "PianoDrawer.h"
#include "NoteGridDrawer.h"
#include <iostream>
#include <fstream>
#include <array>
#include <stack>
#include <unordered_map>
#include <mutex>
#include <queue>

constexpr unsigned int samplesPerSecond{ 44100 };

struct speakerState
{
	Synthesizer* synthesizer;
};

void updateCam(GLFWwindow* window, Cam& cam, float& fov, const float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_EQUAL))
	{
		fov += deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_MINUS))
	{
		fov -= deltaTime;
	}

	fov = glm::mod(fov, 2.0f * glm::pi<float>());

	glm::vec3 transVec{  };
	if (glfwGetKey(window, GLFW_KEY_W))
		transVec.z -= deltaTime;
	if (glfwGetKey(window, GLFW_KEY_A))
		transVec.x -= deltaTime;
	if (glfwGetKey(window, GLFW_KEY_S))
		transVec.z += deltaTime;
	if (glfwGetKey(window, GLFW_KEY_D))
		transVec.x += deltaTime;
	cam.translateRel(70.0f * transVec);
}

MidiFile getMidi(std::string piece)
{
	std::ifstream midiFile{ "project/resources/midi/" + piece + ".mid", std::ios::binary };
	std::stringstream midiStr;
	midiFile >> midiStr.rdbuf();
	MidiFile midi{ midiStr.str() };
	return midi;
}
SoundfontFile getSoundfont()
{
	const std::string sft{ "SGM" };
	std::ifstream sftFile{ "project/resources/soundfonts/" + sft + ".sf2", std::ios::binary };
	std::stringstream sftStr;
	sftFile >> sftStr.rdbuf();
	SoundfontFile soundfont{ sftStr.str() };
	return soundfont;
}

std::queue<Amp> newAmps{  };
std::mutex newAmpsMutex{  };

std::pair<float, float> delayLine(std::pair<float, float>& amp) {
	static constexpr unsigned int M{ 10000 };
	static std::vector<std::pair<float, float>> amps(M, { 0, 0 });
	static unsigned int current{ 0 };
	std::pair<float, float> delayed{ amps[current] };
	amps[current++] = amp;
	if (current >= M) {
		current -= M;
	}
	return delayed;
}

GLFWwindow* window{  };
int playSequence( 
const void* inputBuffer,
void* outputBuffer,
unsigned long frameCount,
const PaStreamCallbackTimeInfo* timeInfo,
PaStreamCallbackFlags statusFlags,
void* userData
)
{
	float* out = (float*)(outputBuffer);
	speakerState* data = (speakerState*)(userData);


	for (unsigned int iii{ 0 }; iii < frameCount; ++iii)
	{
		if (!data->synthesizer->isPlaying())
		{
			data->synthesizer->replay();
		}
		StereoPair next{ data->synthesizer->nextAmp() };
		//std::pair<float, float> delayed{ delayLine(next) };
		//next.first += delayed.first;
		//next.second += delayed.second;
		newAmpsMutex.lock();
		newAmps.emplace(data->synthesizer->getMidiTicksSincePlayStart(), next.right, next.left);
		newAmpsMutex.unlock();
		*out++ = next.right;
		*out++ = next.left;
	}
	return 0;
}

std::vector<VisualNote> turnIntoGroupedNotes(MidiFile midi)
{
	std::vector<VisualNote> visualNotes;
	unsigned int absoluteTime{ 0 };
	
	using Track = MidiFileCore::Track;
	midi.eventBehaviorTable.note.doWhenVisualized([&](const Track::NoteEvent* event)
	{
		visualNotes.emplace_back(event->key, event->channel, absoluteTime, event->endTime);
	});
	for (Track& track : midi.tracks)
	{
		absoluteTime = 0;
		for (unsigned int eventIndex{ 0 }; eventIndex < track.eventCount(); ++eventIndex)
		{
			absoluteTime += track[eventIndex].deltaTime();
			midi.visualize(track[eventIndex]);
		}
	}
	
	return visualNotes;
}

int main(int argc, char** argv)
{
	if (argc <= 1) {
		std::cout << "missing name argument\n";
		return 0;
	}
	Synthesizer synthesizer{ getMidi(argv[1]), getSoundfont(), samplesPerSecond };
	synthesizer.setSamplesPerSecond(samplesPerSecond);
	initialize(&window);
	Cam cam{ glm::vec3(0.0f, 50.0f, 100.0f) };

	glm::dvec2 cursorPos{  };
	glfwGetCursorPos(window, &(cursorPos.x), &(cursorPos.y));

	AmpDrawer ampDrawer;
	FreqDrawer freqDrawer;
	NoteGridDrawer noteGridDrawer;
	PianoDrawer pianoDrawer;
	VisualNoteDrawer visualNoteDrawer;
	visualNoteDrawer.bake(turnIntoGroupedNotes(synthesizer.getLoadedMidi()));
	
	FT_Library freetype;
	if (FT_Init_FreeType(&freetype) != FT_NO_ERROR)
	{
		std::cout << "Failure initializing freetype\n";
		throw 1;
	}
	Singer singer{ freetype, "project/resources/fonts/arial.ttf" };
	synthesizer.doLyricEvent = [&](const std::string& lyric) { singer.singLyric(lyric); };
	FT_Done_FreeType(freetype);

	PaError paErr = Pa_Initialize();
	if (paErr != paNoError)
		std::cout << "Error with PortAudio initialization: " << Pa_GetErrorText(paErr);

	PaStream* stream;
	speakerState audioStreamData{ };
	audioStreamData.synthesizer = &synthesizer;

	paErr = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, samplesPerSecond, 0, playSequence, &audioStreamData);
	//PaStreamParameters streamParameters{ };
	//streamParameters.device = 0;
	//streamParameters.channelCount = 2;
	//streamParameters.sampleFormat = paFloat32;
	//streamParameters.suggestedLatency = 0.1;
	//paErr = Pa_OpenStream(&stream, 0, &streamParameters, samplesPerSecond, 0, paClipOff, playSequence, &audioStreamData);
	
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	while (!glfwWindowShouldClose(window))
	{
		{
			static bool playing{ false };
			if (glfwGetKey(window, GLFW_KEY_SPACE) && !playing)
			{
				playing = true;
				Pa_StartStream(stream);
			}
		}
		static float lastTime{ float(glfwGetTime()) };
		float deltaTime{ float(glfwGetTime()) - lastTime };
		lastTime = float(glfwGetTime());
		processInput(window);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		static float fov{ glm::pi<float>() / 4.0f };
		glm::mat4 projection{ glm::perspective(fov, 1.0f, 0.1f, 400000.0f) };

		updateCam(window, cam, fov, deltaTime);

		glm::dvec2 cursorPosPast{ cursorPos };
		glfwGetCursorPos(window, &(cursorPos.x), &(cursorPos.y));

		glm::dvec2 dCursorPos{ cursorPos - cursorPosPast };

		cam.turnRel({ 0.0f, 1.0f, 0.0f }, deltaTime * float(-dCursorPos.x) / 20.0f);
		cam.turnRel({ 1.0f, 0.0f, 0.0f }, deltaTime * float(-dCursorPos.y) / 20.0f);
		float zTurn{ 0.0f };
		if (glfwGetKey(window, GLFW_KEY_E))
			zTurn -= deltaTime;
		if (glfwGetKey(window, GLFW_KEY_Q))
			zTurn += deltaTime;
		cam.turnRel({ 0.0f, 0.0f, 1.0f }, zTurn);



		std::queue<Amp> newAmpsLocal{  };
		newAmpsMutex.lock();
		std::swap(newAmpsLocal, newAmps);
		newAmpsMutex.unlock();
		while (!newAmpsLocal.empty()) {
			ampDrawer.add(newAmpsLocal.front());
			freqDrawer.add(newAmpsLocal.front());
			newAmpsLocal.pop();
		}

		std::vector<Synthesizer::ActiveNote> activeNotes{ synthesizer.getPlayingNoteTree() };
		
		ampDrawer.draw(projection, cam, synthesizer.getMidiTicksSincePlayStart());
		freqDrawer.nextFrame(synthesizer.getMidiTicksSincePlayStart(), samplesPerSecond);
		freqDrawer.draw(projection, cam, synthesizer.getMidiTicksSincePlayStart());
		visualNoteDrawer.draw(projection, cam, synthesizer.getMidiTicksSincePlayStart());
		noteGridDrawer.draw(projection, cam, synthesizer.getSecondsSincePlayStart(), activeNotes);
		pianoDrawer.draw(projection, cam, activeNotes);

		singer.drawLyrics({ 0.0f, 0.0f });


		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	Pa_CloseStream(stream);
	Pa_Terminate();

	glfwTerminate();
	return 0;
}
