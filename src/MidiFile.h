#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <bitset>
#include <functional>
#include <iostream>
#include <stack>
#include <array>

#ifndef MIDI_FILE
#define MIDI_FILE

#define loop(n) for(unsigned int _{ n }; _ > 0; --_)

class MidiFileCore
{
private:
	class Stream
	{
	protected:
		const std::string_view stream;
		unsigned int cursor;
	public:
		Stream(const std::string_view& stream)
			:stream(stream), cursor{ 0 }
		{
		}
		char peek() const
		{
			return stream[cursor];
		}
		std::string_view peek(const unsigned int length) const
		{
			return stream.substr(cursor, length);
		}
		void advance(const unsigned int length = 1)
		{
			cursor += length;
		}
		char pop()
		{
			const char consumedChar{ this->peek() };
			this->advance();
			return consumedChar;
		}
		std::string_view pop(const unsigned int length)
		{
			const std::string_view consumedText{ this->peek(length) };
			this->advance(length);
			return consumedText;
		}
		bool isEmpty() const
		{
			return cursor >= stream.size();
		}
	};


	class MidiStream : public Stream
	{
	public:
		MidiStream(const std::string_view& stream)
			:Stream(stream)
		{
		}
		unsigned int popVariableLengthQuantity()
		{
			unsigned int vlq{ 0 };
			

			while (true)
			{
				const unsigned char digit{ (unsigned char)(this->pop()) };
				vlq <<= 7;
				vlq += (digit & 0b0111'1111);
				if ((digit & 0b1000'0000) == 0)
					break;
			}
			return vlq;
		}
	};
	
	
public:
	struct Header
	{
		struct TimeDivision
		{
			enum Format
			{
				metrical,
				timecode
			};
			virtual Format getFormat() const = 0;
			virtual TimeDivision* clone() const = 0;
			virtual ~TimeDivision() {  }
		};
		struct MetricalTimeDivision : TimeDivision
		{
			unsigned int ticksPerQuarterNote;
			MetricalTimeDivision(const unsigned int division)
				:ticksPerQuarterNote(division)
			{
			}
			Format getFormat() const override
			{
				return Format::metrical;
			}
			TimeDivision* clone() const override
			{
				return new MetricalTimeDivision(*this);
			}
		};
		struct TimecodeBasedTimeDivision : TimeDivision
		{
			int negativeSMPTEFormat;
			unsigned int ticksPerFrame;
			TimecodeBasedTimeDivision(const unsigned int division)
				:negativeSMPTEFormat(int(((division >> 1) ^ 0b1111'1111) + 1)), ticksPerFrame(division & 0b1111'1111)
			{
				std::cout << std::hex << division; while (true);
			}
			Format getFormat() const override
			{
				return Format::timecode;
			}
			TimeDivision* clone() const override
			{
				return new TimecodeBasedTimeDivision(*this);
			}
		};
		unsigned int numberOfTracks;
		std::unique_ptr<TimeDivision> timeDivision;
private:
		void parseFormatFrom(MidiStream& stream)
		{
			stream.advance(2);
		}

		void parseNumberOfTracksFrom(MidiStream& stream)
		{
			this->numberOfTracks = uInt::fromAsciiString(stream.pop(2));
		}

		void parseTimeDivisionFrom(MidiStream& stream)
		{
			const unsigned int division{ uInt::fromAsciiString(stream.pop(2)) };
			if ((division & 0b1000'0000'0000'0000) == 0)
			{
				this->timeDivision = std::make_unique<MetricalTimeDivision>(division);
			}
			else
			{
				this->timeDivision = std::make_unique<TimecodeBasedTimeDivision>(division);
			}
		}
public:
		Header(const std::string_view& header)
		{
			//TODO: abstract away all parsing in a similar way to this (i. e. with helper functions)
			MidiStream midi{ header };
			this->parseFormatFrom(midi);
			this->parseNumberOfTracksFrom(midi);
			this->parseTimeDivisionFrom(midi);
		}
		Header(const Header& header)
			:
			numberOfTracks(header.numberOfTracks),
			timeDivision{ std::unique_ptr<TimeDivision>{ header.timeDivision->clone() } }
		{
		}
		Header()
			:numberOfTracks(0)
		{
		}
		void operator=(const Header& header)
		{
			numberOfTracks = header.numberOfTracks;
			timeDivision = std::unique_ptr<TimeDivision>{ header.timeDivision->clone() };
		}
	};







	enum class Error
	{
		UnreadableSequence
	};
	MidiFileCore(const std::string_view& midi)
	{
		MidiStream midiStream{ midi };


		const std::string_view MThdSignature{ midiStream.pop(4) };

		if (MThdSignature != "MThd")
		{
			throw Error::UnreadableSequence;
		}
		else
		{
			const unsigned int headerLength{ uInt::fromAsciiString(midiStream.pop(4)) };
			header = Header(midiStream.pop(headerLength));
			loop(header.numberOfTracks)
			{
				const std::string_view MTrkSignature{ midiStream.pop(4) };
				if (MTrkSignature != "MTrk")
				{
					throw Error::UnreadableSequence;
				}
				const unsigned int trackLength{ uInt::fromAsciiString(midiStream.pop(4)) };
				const Track track{ midiStream.pop(trackLength) };
				tracks.push_back(track);
			}
		}
	}


	struct EventBehaviorTable;
	struct Track
	{
	public:

		struct EventBase
		{
			unsigned int deltaTime;
			virtual void play(const EventBehaviorTable& behaviorTable) const = 0;
			virtual void visualize(const EventBehaviorTable& behaviorTable) const = 0;
			virtual EventBase* clone() const = 0;
			virtual ~EventBase() {  }
		};
		struct NoteOffEvent : EventBase
		{
			unsigned char key, velocity, channel;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.noteOff.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.noteOff.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct NoteOnEvent : EventBase
		{
			unsigned char key, velocity, channel;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.noteOn.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.noteOn.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct ControlChangeEvent : EventBase
		{
			unsigned int controller, value, channel;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.controlChange.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.controlChange.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct ProgramChangeEvent : EventBase
		{
			unsigned int program, channel;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.programChange.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.programChange.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct PitchBendEvent : EventBase
		{
			unsigned int leastSignificant7Bits, mostSignificant7Bits, channel;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.pitchBend.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.pitchBend.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct SystemExclusiveMessageEvent : EventBase
		{
			std::string message;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.systemExclusiveMessage.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.systemExclusiveMessage.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct LyricEvent : EventBase
		{
			std::string lyric;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.lyric.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.lyric.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct TempoChangeEvent : EventBase
		{
			unsigned int microsecondsPerQuarterNote;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.tempoChange.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.tempoChange.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct UnknownEvent : EventBase
		{
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.unknown.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.unknown.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		struct NoteEvent : EventBase //TODO: move NoteEvent from MidiFileCore to MidiFile
		{
			unsigned char key, velocity, channel;
			unsigned int endTime;
			void play(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.note.play(this);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const override
			{
				behaviorTable.note.visualize(this);
			}
			EventBase* clone() const override
			{
				return new auto(*this);
			}
		};
		class Event
		{
		private:
			std::unique_ptr<EventBase> event;
		public:
			Event(const EventBase* event)
				:event(event->clone())
			{

			}
			Event(const Event& other)
				:Event(other.event.get())
			{
			}
			Event& operator=(const Event& other)
			{
				event = std::unique_ptr<EventBase>{ other.event->clone() };
				return *this;
			}
			unsigned int& deltaTime() const
			{
				return event->deltaTime;
			}
			void play(const EventBehaviorTable& behaviorTable) const
			{
				event->play(behaviorTable);
			}
			void visualize(const EventBehaviorTable& behaviorTable) const
			{
				event->visualize(behaviorTable);
			}
			EventBase* get() const
			{
				return event.get();
			}
		};

	private:
		std::vector<Event> events;
	public:
		Event& operator[](const unsigned int index)
		{
			return events[index];
		}
		unsigned int eventCount() const
		{
			return events.size();
		}

		Track(const std::vector<Event>& events)
		{
			this->events.reserve(events.size());
			for (const Event& event : events)
			{
				this->events.emplace_back(event);
			}
		}
		Track(const Track& other)
			:Track(other.events)
		{

		}

		Track(const std::string_view& track)
		{
			MidiStream trackStream{ track };
			
			unsigned int runningStatus{ 0 };

			while (!trackStream.isEmpty())
			{
				unsigned int eventDeltaTime{ trackStream.popVariableLengthQuantity() };
				unsigned char eventDataFirstByte{ (unsigned char)(trackStream.peek()) };

				if ((eventDataFirstByte & 0b1000'0000) == 0)
				{
					eventDataFirstByte = runningStatus;
				}
				else
				{
					trackStream.advance();
					runningStatus = eventDataFirstByte;
				}

				switch (eventDataFirstByte)
				{
				case 0xff:
				{
					unsigned char eventType{ (unsigned char)(trackStream.pop()) };
					unsigned int eventLength{ trackStream.popVariableLengthQuantity() };
					std::string eventContents{ trackStream.pop(eventLength) };
					switch (eventType)
					{
					case 0x51:
					{
						TempoChangeEvent event;
						event.deltaTime = eventDeltaTime;
						event.microsecondsPerQuarterNote = uInt::fromAsciiString(eventContents);
						events.emplace_back(&event);
					}break;
					case 0x05:
					{
						LyricEvent event;
						event.deltaTime = eventDeltaTime;
						event.lyric = eventContents;
						events.emplace_back(&event);
					}break;
					default:
					{
						UnknownEvent event;
						event.deltaTime = eventDeltaTime;
						events.emplace_back(&event);
					}
					}
				}break;
				case 0xf0:
				{
					SystemExclusiveMessageEvent event;
					event.deltaTime = eventDeltaTime;
					unsigned char sysexByte{ (unsigned char)(trackStream.pop()) };
					while (sysexByte != 0xf7)
					{
						event.message += sysexByte;
						sysexByte = trackStream.pop();
					}
					events.emplace_back(&event);
				}break;
				default:
				{
					switch (eventDataFirstByte & 0b1111'0000)
					{
					case 0b1000'0000:
					{
						NoteOffEvent event;
						event.deltaTime = eventDeltaTime;
						event.channel = eventDataFirstByte & 0b0000'1111;
						event.key = trackStream.pop();
						event.velocity = trackStream.pop();
						events.emplace_back(&event);
						break;
					}
					case 0b1001'0000:
					{
						NoteOnEvent event;
						event.deltaTime = eventDeltaTime;
						event.channel = eventDataFirstByte & 0b0000'1111;
						event.key = trackStream.pop();
						event.velocity = trackStream.pop();
						if (event.velocity != 0)
							events.emplace_back(&event);
						else
						{
							NoteOffEvent offEvent;
							offEvent.deltaTime = event.deltaTime;
							offEvent.channel = event.channel;
							offEvent.key = event.key;
							offEvent.velocity = event.velocity;
							events.emplace_back(&offEvent);
						}
						break;
					}
					case 0b1100'0000:
					{
						ProgramChangeEvent event;
						event.deltaTime = eventDeltaTime;
						event.channel = eventDataFirstByte & 0b0000'1111;
						event.program = trackStream.pop();
						events.emplace_back(&event);
						break;
					}
					case 0b1110'0000:
					{
						PitchBendEvent event;
						event.deltaTime = eventDeltaTime;
						event.channel = eventDataFirstByte & 0b0000'1111;
						event.leastSignificant7Bits = trackStream.pop();
						event.mostSignificant7Bits = trackStream.pop();
						events.emplace_back(&event);
						break;
					}
					case 0b1011'0000:
					{
						ControlChangeEvent event;
						event.deltaTime = eventDeltaTime;
						event.channel = eventDataFirstByte & 0b0000'1111;
						event.controller = trackStream.pop();
						event.value = trackStream.pop();
						events.emplace_back(&event);
						break;
					}
					default:
					{
						std::cerr << std::bitset<8>(eventDataFirstByte) << " - unrecognized event status byte\n";
						throw Error::UnreadableSequence;
					}
					}
				}
				}
			}
		}
		Track()
		{
		}
	};

	template<class T>
	struct EventBehavior
	{
		std::function<void(const T*)> play{ [](auto) {} };
		std::function<void(const T*)> visualize{ [](auto) {} };
		void doWhenPlayed(const std::function<void(const T*)>& play)
		{
			this->play = play;
		}
		void doWhenVisualized(const std::function<void(const T*)>& visualize)
		{
			this->visualize = visualize;
		}
	};


	struct EventBehaviorTable
	{
		EventBehavior<Track::NoteEvent> note;//TODO: move note event behavior from MidiFileCore to MidiFile
		EventBehavior<Track::NoteOffEvent> noteOff;
		EventBehavior<Track::NoteOnEvent> noteOn;
		EventBehavior<Track::ControlChangeEvent> controlChange;
		EventBehavior<Track::ProgramChangeEvent> programChange;
		EventBehavior<Track::PitchBendEvent> pitchBend;
		EventBehavior<Track::SystemExclusiveMessageEvent> systemExclusiveMessage;
		EventBehavior<Track::LyricEvent> lyric;
		EventBehavior<Track::TempoChangeEvent> tempoChange;
		EventBehavior<Track::UnknownEvent> unknown;
	};
	EventBehaviorTable eventBehaviorTable;
public:
	void play(const Track::Event& event)
	{
		event.play(eventBehaviorTable);
	}
	void visualize(const Track::Event& event)
	{
		event.visualize(eventBehaviorTable);
	}

	Header header;
	std::vector<Track> tracks;


private:



	class uInt
	{
	public:
		unsigned int static fromAsciiString(const std::string_view& str)
		{
			unsigned int translated{ 0 };
			for (const unsigned char digit : str)
			{
				translated <<= 8;
				translated += digit;
			}
			return translated;
		}
	};
};

class MidiFile
{
public:

	std::unique_ptr<MidiFileCore::Header::TimeDivision> timeDivision;
	MidiFileCore::EventBehaviorTable eventBehaviorTable;
	std::vector<MidiFileCore::Track> tracks;
private:
	void turnNoteOnOffPairsIntoNoteEvents()
	{
		unsigned int absoluteTime{ 0 };

		using Track = MidiFileCore::Track;
		std::array<std::array<std::stack<unsigned int>, 128>, 16> keyboard;
		std::vector<Track::Event> events;
		eventBehaviorTable.controlChange.doWhenPlayed([&](const Track::ControlChangeEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.lyric.doWhenPlayed([&](const Track::LyricEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.pitchBend.doWhenPlayed([&](const Track::PitchBendEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.programChange.doWhenPlayed([&](const Track::ProgramChangeEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.systemExclusiveMessage.doWhenPlayed([&](const Track::SystemExclusiveMessageEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.tempoChange.doWhenPlayed([&](const Track::TempoChangeEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.unknown.doWhenPlayed([&](const Track::UnknownEvent* event)
			{
				events.emplace_back(event);
			});
		eventBehaviorTable.noteOff.doWhenPlayed([&](const Track::NoteOffEvent* event)
			{
				if (keyboard[event->channel][event->key].size() == 0) {
					std::cout << "found a noteoff with no corresponding noteon. Ignoring.\n";
					return;
				}
				Track::Event& correspondingEvent{ events[keyboard[event->channel][event->key].top()] };
				
				Track::NoteEvent* note{ static_cast<Track::NoteEvent*>(correspondingEvent.get()) };
				note->endTime = absoluteTime;
				
				keyboard[event->channel][event->key].pop();

				Track::NoteOffEvent dummyNoteOff;
				dummyNoteOff.channel = event->channel;
				dummyNoteOff.deltaTime = event->deltaTime;
				dummyNoteOff.key = event->key;
				dummyNoteOff.velocity = event->velocity;
				events.emplace_back(&dummyNoteOff);
			});
		eventBehaviorTable.noteOn.doWhenPlayed([&](const Track::NoteOnEvent* event)
			{
				Track::NoteEvent newNote;
				newNote.channel = event->channel;
				newNote.deltaTime = event->deltaTime;
				newNote.key = event->key;
				newNote.velocity = event->velocity;
				events.emplace_back(&newNote);
				keyboard[event->channel][event->key].push(events.size() - 1);
			});
		for (Track& track : this->tracks)
		{
			absoluteTime = 0;
			for (unsigned int eventIndex{ 0 }; eventIndex < track.eventCount(); ++eventIndex)
			{
				absoluteTime += track[eventIndex].deltaTime();
				this->play(track[eventIndex]);
			}
			track = Track{ events };
			events.clear();
		}
		eventBehaviorTable.controlChange.doWhenPlayed([&](const Track::ControlChangeEvent* event)
			{});
		eventBehaviorTable.lyric.doWhenPlayed([&](const Track::LyricEvent* event)
			{});
		eventBehaviorTable.pitchBend.doWhenPlayed([&](const Track::PitchBendEvent* event)
			{});
		eventBehaviorTable.programChange.doWhenPlayed([&](const Track::ProgramChangeEvent* event)
			{});
		eventBehaviorTable.systemExclusiveMessage.doWhenPlayed([&](const Track::SystemExclusiveMessageEvent* event)
			{});
		eventBehaviorTable.tempoChange.doWhenPlayed([&](const Track::TempoChangeEvent* event)
			{});
		eventBehaviorTable.unknown.doWhenPlayed([&](const Track::UnknownEvent* event)
			{});
		eventBehaviorTable.noteOff.doWhenPlayed([&](const Track::NoteOffEvent* event)
			{});
		eventBehaviorTable.noteOn.doWhenPlayed([&](const Track::NoteOnEvent* event)
			{});
	}
public:
	MidiFile(const std::string_view& midi)
	{
		const MidiFileCore data{ midi };
		this->timeDivision = std::unique_ptr<MidiFileCore::Header::TimeDivision>{ data.header.timeDivision->clone() };
		this->tracks = data.tracks;
		this->turnNoteOnOffPairsIntoNoteEvents();
	}
	MidiFile(const MidiFile& other)
	{
		this->timeDivision = std::unique_ptr<MidiFileCore::Header::TimeDivision>{ other.timeDivision->clone() };
		this->eventBehaviorTable = other.eventBehaviorTable;
		this->tracks = other.tracks;
	}

	void play(const MidiFileCore::Track::Event& event)
	{
		event.play(eventBehaviorTable);
	}
	void visualize(const MidiFileCore::Track::Event& event)
	{
		event.visualize(eventBehaviorTable);
	}
};

#endif
