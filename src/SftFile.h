#include <cstdint>
#include <vector>
#include <string_view>
#include <array>
#include <map>

#ifndef SFT_READER
#define SFT_READER

class SoundfontFile
{
public:
	typedef uint8_t Byte;
	typedef uint16_t Word;
	typedef uint32_t Dword;
	typedef int8_t Char;
	typedef int16_t Short;
private:
	struct Chunk
	{
		std::string id;
		std::string data;
	};
	struct Riff
	{
	private:
		std::string_view id{ "RIFF" };
	public:
		std::string type;
		std::string data;
		Riff(const Chunk& chunk)
		{
			if (chunk.id != this->id)
				throw 0;
			Stream dataStream{ chunk.data };
			this->type = dataStream.pop(4);
			this->data = dataStream.pop(chunk.data.length() - 4);
		}
	};
	struct List
	{
	private:
		std::string_view id{ "LIST" };
	public:
		std::string type;
		std::string data;
		List(const Chunk& chunk)
		{
			if (chunk.id != this->id)
				throw 0;
			Stream dataStream{ chunk.data };
			this->type = dataStream.pop(4);
			this->data = dataStream.pop(chunk.data.length() - 4);
		}
	};
	struct Bank
	{
	public:
		struct InfoChunk
		{
		private:
			std::string_view type{ "INFO" };
		public:
			struct VersionChunk
			{
			private:
				std::string_view id{ "ifil" };
			public:
				Word major, minor;
				VersionChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					SoundfontStream dataStream{ chunk.data };
					major = uInt::fromAsciiString(dataStream.pop(2));
					minor = uInt::fromAsciiString(dataStream.pop(2));
				}
				VersionChunk()
				{

				}
			};
			struct TargetEngineChunk
			{
			private:
				std::string_view id{ "isng" };
			public:
				std::string targetEngine;
				TargetEngineChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					targetEngine = chunk.data;
				}
				TargetEngineChunk()
				{

				}
			};
			struct NameChunk
			{
			private:
				std::string_view id{ "INAM" };
			public:
				std::string name;
				NameChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					name = chunk.data;
				}
				NameChunk()
				{

				}
			};
			VersionChunk versionChunk;
			TargetEngineChunk targetEngineChunk;
			NameChunk nameChunk;
		private:
			void assignChunk(const Chunk& chunk)
			{
				if (chunk.id == "ifil")
				{
					versionChunk = VersionChunk(chunk);
				}
				else if (chunk.id == "isng")
				{
					targetEngineChunk = TargetEngineChunk(chunk);
				}
				else if (chunk.id == "INAM")
				{
					nameChunk = NameChunk(chunk);
				}
				else if (chunk.id == "irom")
				{

				}
				else if (chunk.id == "iver")
				{

				}
				else if (chunk.id == "ICRD")
				{

				}
				else if (chunk.id == "IENG")
				{

				}
				else if (chunk.id == "IPRD")
				{

				}
				else if (chunk.id == "ICOP")
				{

				}
				else if (chunk.id == "ICMT")
				{

				}
				else if (chunk.id == "ISFT")
				{

				}
			}
		public:
			InfoChunk(const List& list)
			{
				if (list.type != this->type)
					throw 0;
				SoundfontStream dataStream{ list.data };
				while (!dataStream.isEmpty())
				{
					const Chunk chunk{ dataStream.popChunk() };
					assignChunk(chunk);
				}
			}
			InfoChunk()
			{

			}
		};
		struct SampleDataChunk
		{
		private:
			std::string_view type{ "sdta" };
		public:
			struct smplChunk
			{
			private:
				std::string_view id{ "smpl" };
			public:
				std::vector<int16_t> sampleDataPoints;
				smplChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;

					sampleDataPoints.reserve(chunk.data.size() / 2);
					for (unsigned long long i{ 0 }; i < chunk.data.size(); i += 2) {
						sampleDataPoints.push_back(Int::fromTwoAsciiCharsLE(chunk.data[i], chunk.data[i + 1]));
					}
					// sample loading can also be implemented with a Stream, but that's currently slower
					//Stream dataStream{ chunk.data };
					//while (!dataStream.isEmpty())
					//{
					//	sampleDataPoints.push_back(Int::fromTwoAsciiChars(dataStream.popChar(), dataStream.popChar()));
					//}
				}
				smplChunk()
				{

				}
			};
			struct smpl24Chunk
			{
			private:
				std::string_view id{ "smpl24" };
			public:
				std::vector<Byte> sampleDataPoints;
				smpl24Chunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					sampleDataPoints.reserve(chunk.data.size() - (chunk.data.size() % 2));
					while (!dataStream.isEmpty())
					{
						sampleDataPoints.push_back(uInt::fromAsciiString(dataStream.pop(1)));
					}
				}
				smpl24Chunk()
				{

				}
			};
			std::vector<int> sampleDataPoints;
			SampleDataChunk(const List& list)
			{
				if (list.type != this->type)
					throw 0;
				SoundfontStream dataStream{ list.data };
				if (!dataStream.isEmpty())
				{
					smplChunk sampleDataPointsHighWords{ dataStream.popChunk() };
					sampleDataPoints.resize(sampleDataPointsHighWords.sampleDataPoints.size());
					for (unsigned int i{ 0 }; i < sampleDataPointsHighWords.sampleDataPoints.size(); ++i)
						sampleDataPoints[i] = ((int)(sampleDataPointsHighWords.sampleDataPoints[i]) << 8);
				}
				if (!dataStream.isEmpty())
				{
					smpl24Chunk sampleDataPointsLowBytes{ dataStream.popChunk() };
					for (unsigned int i{ 0 }; i < sampleDataPoints.size(); ++i)
					{
						sampleDataPoints[i] += (sampleDataPoints[i] >= 0 ? 1 : -1) * sampleDataPointsLowBytes.sampleDataPoints[i];
					}
				}
			}
			SampleDataChunk()
			{

			}
		};
		struct PresetDataChunk
		{
		private:
			std::string_view type{ "pdta" };
		public:
			typedef unsigned int ModulatorEnum;
			typedef unsigned int GeneratorEnum;
			typedef unsigned int TransformEnum;

			struct Generator
			{
				union GeneratorAmount
				{
					struct Range
					{
						Byte low, high;
					};
					Range range;
					Short shortAmount;
					Word wordAmount;
				};
				GeneratorEnum generator;
				GeneratorAmount generatorAmount;
				Generator(const std::string_view& data)
				{
					Stream dataStream{ data };
					generator = uInt::fromAsciiString(dataStream.pop(2));
					switch (generator)
					{
					case 41:
						generatorAmount.wordAmount = uInt::fromAsciiString(dataStream.pop(2));
						break;
					case 43:
					case 44:
						generatorAmount.range.low = uInt::fromAsciiString(dataStream.pop(1));
						generatorAmount.range.high = uInt::fromAsciiString(dataStream.pop(1));
						break;
					default: generatorAmount.shortAmount = Int::fromAsciiString(dataStream.pop(2)); break;
					}
				}
				Generator()
				{

				}
			};

			struct Modulator
			{
				ModulatorEnum source;
				GeneratorEnum destination;
				int modulationAmount;
				ModulatorEnum amountSource;
				TransformEnum transform;
				Modulator(const std::string_view& data)
				{
					Stream dataStream{ data };
					source = uInt::fromAsciiString(dataStream.pop(2));
					destination = uInt::fromAsciiString(dataStream.pop(2));
					modulationAmount = uInt::fromAsciiString(dataStream.pop(2));
					amountSource = uInt::fromAsciiString(dataStream.pop(2));
					transform = uInt::fromAsciiString(dataStream.pop(2));
				}
				Modulator()
				{

				}
			};

			struct Bag
			{
				unsigned int generatorIndex, modulatorIndex;
				Bag(const std::string_view& data)
				{
					generatorIndex = uInt::fromAsciiString(data.substr(0, 2));
					modulatorIndex = uInt::fromAsciiString(data.substr(2, 2));
				}
			};

			struct PresetHeaderChunk
			{
			private:
				std::string_view id{ "phdr" };
			public:
				struct PresetHeader
				{
					std::string name;
					Word presetNumber;
					Word bankNumber;
					Word bagIndex;
					Dword library, genre, morphology;
					PresetHeader(const std::string_view& data)
					{
						Stream dataStream{ data };
						name = dataStream.pop(20).data();
						presetNumber = uInt::fromAsciiString(dataStream.pop(2));
						bankNumber = uInt::fromAsciiString(dataStream.pop(2));
						bagIndex = uInt::fromAsciiString(dataStream.pop(2));
						library = uInt::fromAsciiString(dataStream.pop(4));
						genre = uInt::fromAsciiString(dataStream.pop(4));
						morphology = uInt::fromAsciiString(dataStream.pop(4));
					}
				};
				std::vector<PresetHeader> presetHeaders;
				PresetHeaderChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					presetHeaders.reserve(chunk.data.size() / 38);
					while (!dataStream.isEmpty())
					{
						presetHeaders.emplace_back(dataStream.pop(38));
					}
				}
				PresetHeaderChunk()
				{

				}
			};
			struct PresetBagChunk
			{
			private:
				std::string_view id{ "pbag" };
			public:
				std::vector<Bag> bags;
				PresetBagChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					bags.reserve(chunk.data.size() / 4);
					while (!dataStream.isEmpty())
					{
						bags.push_back(dataStream.pop(4));
					}
				}
				PresetBagChunk()
				{

				}
			};

			struct PresetModulatorChunk
			{
			private:
				std::string_view id{ "pmod" };
			public:
				std::vector<Modulator> modulators;
				PresetModulatorChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					modulators.reserve(chunk.data.size() / 10);
					while (!dataStream.isEmpty())
					{
						modulators.push_back(dataStream.pop(10));
					}
				}
				PresetModulatorChunk()
				{

				}
			};

			struct PresetGeneratorChunk
			{
			private:
				std::string_view id{ "pgen" };
			public:
				std::vector<Generator> generators;
				PresetGeneratorChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					generators.reserve(chunk.data.size() / 4);
					while (!dataStream.isEmpty())
					{
						generators.push_back(dataStream.pop(4));
					}
				}
				PresetGeneratorChunk()
				{

				}
			};

			struct InstrumentChunk
			{
			private:
				std::string_view id{ "inst" };
			public:
				struct Instrument
				{
					std::string name;
					Word bagIndex;

					Instrument(const std::string_view& data)
					{
						name = data.substr(0, 20).data();
						bagIndex = uInt::fromAsciiString(data.substr(20, 2));
					}
					Instrument()
					{

					}
				};
				std::vector<Instrument> instruments;
				InstrumentChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					instruments.reserve(chunk.data.size() / 22);
					while (!dataStream.isEmpty())
					{
						instruments.emplace_back(dataStream.pop(22));
					}
				}
				InstrumentChunk()
				{

				}
			};

			struct InstrumentBagChunk
			{
			private:
				std::string_view id{ "ibag" };
			public:
				std::vector<Bag> bags;
				InstrumentBagChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					bags.reserve(chunk.data.size() / 4);
					while (!dataStream.isEmpty())
					{
						bags.push_back(dataStream.pop(4));
					}
				}
				InstrumentBagChunk()
				{

				}
			};

			struct InstrumentModulatorChunk
			{
			private:
				std::string_view id{ "imod" };
			public:
				std::vector<Modulator> modulators;
				InstrumentModulatorChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					modulators.reserve(chunk.data.size() / 10);
					while (!dataStream.isEmpty())
					{
						modulators.push_back(dataStream.pop(10));
					}
				}
				InstrumentModulatorChunk()
				{

				}
			};

			struct InstrumentGeneratorChunk
			{
			private:
				std::string_view id{ "igen" };
			public:
				std::vector<Generator> generators;
				InstrumentGeneratorChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					generators.reserve(chunk.data.size() / 4);
					while (!dataStream.isEmpty())
					{
						generators.push_back(dataStream.pop(4));
					}
				}
				InstrumentGeneratorChunk()
				{

				}
			};

			struct SampleHeaderChunk
			{
			private:
				std::string_view id{ "shdr" };
			public:
				struct SampleHeader
				{
					std::string name;
					Dword start, end;
					Dword loopStart, loopEnd;
					Dword sampleRate;
					Byte originalPitch;
					Char pitchCorrection;
					Word link;
					enum class SampleType
					{
						mono = 0b0001,
						right = 0b0010,
						left = 0b0100,
						linked = 0b1000,
						romMono = mono | 0x8000,
						romRight = right | 0x8000,
						romLeft = left | 0x8000,
						romLinked = linked | 0x8000,
					};
					SampleType type;
					SampleHeader(const std::string_view& data)
					{
						Stream dataStream{ data };
						name = dataStream.pop(20).data();
						start = uInt::fromAsciiString(dataStream.pop(4));
						end = uInt::fromAsciiString(dataStream.pop(4));
						loopStart = uInt::fromAsciiString(dataStream.pop(4));
						loopEnd = uInt::fromAsciiString(dataStream.pop(4));
						sampleRate = uInt::fromAsciiString(dataStream.pop(4));
						originalPitch = uInt::fromAsciiString(dataStream.pop(1));
						pitchCorrection = Int::fromAsciiString(dataStream.pop(1));
						link = uInt::fromAsciiString(dataStream.pop(2));
						type = SampleType(uInt::fromAsciiString(dataStream.pop(2)));
					}
					SampleHeader()
					{

					}
				};
				std::vector<SampleHeader> samples;
				SampleHeaderChunk(const Chunk& chunk)
				{
					if (chunk.id != this->id)
						throw 0;
					Stream dataStream{ chunk.data };
					samples.reserve(chunk.data.size() / 46);
					while (!dataStream.isEmpty())
					{
						samples.push_back(dataStream.pop(46));
					}
				}
				SampleHeaderChunk()
				{

				}
			};
			PresetHeaderChunk presetHeaderChunk;
			PresetBagChunk presetBagChunk;
			PresetModulatorChunk presetModulatorChunk;
			PresetGeneratorChunk presetGeneratorChunk;
			InstrumentChunk instrumentChunk;
			InstrumentBagChunk instrumentBagChunk;
			InstrumentModulatorChunk instrumentModulatorChunk;
			InstrumentGeneratorChunk instrumentGeneratorChunk;
			SampleHeaderChunk sampleHeaderChunk;
			PresetDataChunk(const List& list)
			{
				if (list.type != this->type)
					throw 0;
				SoundfontStream dataStream{ list.data };
				this->presetHeaderChunk = PresetHeaderChunk(dataStream.popChunk());
				this->presetBagChunk = PresetBagChunk(dataStream.popChunk());
				this->presetModulatorChunk = PresetModulatorChunk(dataStream.popChunk());
				this->presetGeneratorChunk = PresetGeneratorChunk(dataStream.popChunk());
				this->instrumentChunk = InstrumentChunk(dataStream.popChunk());
				this->instrumentBagChunk = InstrumentBagChunk(dataStream.popChunk());
				this->instrumentModulatorChunk = InstrumentModulatorChunk(dataStream.popChunk());
				this->instrumentGeneratorChunk = InstrumentGeneratorChunk(dataStream.popChunk());
				this->sampleHeaderChunk = SampleHeaderChunk(dataStream.popChunk());
			}
			PresetDataChunk()
			{

			}
		};
	private:
		std::string_view type{ "sfbk" };
	public:
		InfoChunk infoChunk;
		SampleDataChunk sampleDataChunk;
		PresetDataChunk presetDataChunk;
		Bank(const Riff& riff)
		{
			if (riff.type != this->type)
				throw 0;
			SoundfontStream dataStream{ riff.data };
			infoChunk = InfoChunk(dataStream.popList());
			sampleDataChunk = SampleDataChunk(dataStream.popList());
			presetDataChunk = PresetDataChunk(dataStream.popList());
			if (!dataStream.isEmpty())
			{
				throw 0;
			}
		}
		Bank()
		{

		}
	};

	class Stream
	{
	protected:
		const std::string_view data;
		unsigned int cursor;
	public:
		Stream(const std::string_view& data)
			:data(data), cursor{ 0 }
		{
		}
		char peek() const
		{
			return data[cursor];
		}
		std::string_view peek(const unsigned int length) const
		{
			return data.substr(cursor, length);
		}
		void advance(const int length = 1)
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
			return cursor >= data.size();
		}
	};
	class SoundfontStream : public Stream
	{
	public:
		SoundfontStream(const std::string_view& data)
			:Stream(data)
		{
		}
		Chunk popChunk()
		{
			Chunk chunk;
			chunk.id = this->pop(4);
			const unsigned int sizeOfData{ uInt::fromAsciiString(this->pop(4)) };
			chunk.data = this->pop(sizeOfData);
			if (sizeOfData % 2 == 1)
			{
				this->advance();
			}
			return chunk;
		}
		Riff popRiff()
		{
			return Riff(popChunk());
		}
		List popList()
		{
			return List(popChunk());
		}
		Bank popBank()
		{
			return Bank(popRiff());
		}
	};

	class uInt
	{
	public:
		unsigned int static fromAsciiString(const std::string_view& str)
		{
			unsigned int translated{ 0 };
			unsigned int factor{ 1 };
			for (const unsigned char digit : str)
			{
				translated += factor * digit;
				factor <<= 8;
			}
			return translated;
		}
	};
	class Int
	{
	public:
		unsigned int static fromAsciiString(const std::string_view& str)
		{
			int translated{ 0 };
			unsigned int factor{ 1 };
			for (const unsigned char digit : str)
			{
				translated += factor * digit;
				factor <<= 8;
			}
			return translated;
		}
		unsigned int static fromTwoAsciiCharsLE(const unsigned char c1, const unsigned char c2) {
			return c1 + (c2 << 8);
		}
	};
public:

	struct Version
	{
		Version(const Bank::InfoChunk::VersionChunk& versionChunk)
			:major(versionChunk.major), minor(versionChunk.minor)
		{
		}
		Version()
		{
		}
		Word major, minor;
	};

	Version version;
	std::string name;
	std::string targetEngine;

	
	std::vector<int> sampleDataPoints;

	struct Bag
	{
		using Modulator = Bank::PresetDataChunk::Modulator;
		struct Generator
		{
			union GeneratorAmount
			{
				struct Range
				{
					Byte low, high;
				};
				Range range;
				Short shortAmount;
				Word wordAmount;
				GeneratorAmount(const Byte low, const Byte high)
					:range{ low, high }
				{

				}
				GeneratorAmount(const Short amount)
					:shortAmount{ amount }
				{

				}
				GeneratorAmount(const Bank::PresetDataChunk::Generator::GeneratorAmount amount)
					:shortAmount(amount.shortAmount)
				{

				}
				GeneratorAmount()
					:shortAmount(0)
				{

				}
			};

			bool isSet{ false };
			GeneratorAmount amount;
			Generator(const Byte low, const Byte high)
				:amount{ low, high }
			{
				
			}
			Generator(const Short amount)
				:amount{ amount }
			{

			}
			Generator(const Bank::PresetDataChunk::Generator& generator)
				:amount(generator.generatorAmount)
			{

			}
			Generator()
				:isSet(false)
			{

			}
			void set(const Byte low, const Byte high)
			{
				this->amount = { low, high };
				isSet = true;
			}
			void set(const Short amount)
			{
				this->amount = amount;
				isSet = true;
			}
			void set(const Bank::PresetDataChunk::Generator& generator)
			{
				amount = generator.generatorAmount;
				isSet = true;
			}
			void set(const Generator& generator)
			{
				this->amount = generator.amount;
				isSet = true;
			}
		};
		static constexpr unsigned int generatorCount{ 61 };
		std::array<Generator, generatorCount> generators
		{
			/* 0*/Generator{ 0 },
			/* 1*/Generator{ 0 },
			/* 2*/Generator{ 0 },
			/* 3*/Generator{ 0 },
			/* 4*/Generator{ 0 },
			/* 5*/Generator{ 0 },
			/* 6*/Generator{ 0 },
			/* 7*/Generator{ 0 },
			/* 8*/Generator{ 13500 },
			/* 9*/Generator{ 0 },
			/*10*/Generator{ 0 },
			/*11*/Generator{ 0 },
			/*12*/Generator{ 0 },
			/*13*/Generator{ 0 },
			/*14*/Generator{  },
			/*15*/Generator{ 0 },
			/*16*/Generator{ 0 },
			/*17*/Generator{ 0 },
			/*18*/Generator{  },
			/*19*/Generator{  },
			/*20*/Generator{  },
			/*21*/Generator{ -12000 },
			/*22*/Generator{ 0 },
			/*23*/Generator{ -12000 },
			/*24*/Generator{ 0 },
			/*25*/Generator{ -12000 },
			/*26*/Generator{ -12000 },
			/*27*/Generator{ -12000 },
			/*28*/Generator{ -12000 },
			/*29*/Generator{ 0 },
			/*30*/Generator{ -12000 },
			/*31*/Generator{ 0 },
			/*32*/Generator{ 0 },
			/*33*/Generator{ -12000 },
			/*34*/Generator{ -12000 },
			/*35*/Generator{ -12000 },
			/*36*/Generator{ -12000 },
			/*37*/Generator{ 0 },
			/*38*/Generator{ -12000 },
			/*39*/Generator{ 0 },
			/*40*/Generator{ 0 },
			/*41*/Generator{  },
			/*42*/Generator{  },
			/*43*/Generator{ 0, 127 },
			/*44*/Generator{ 0, 127 },
			/*45*/Generator{ 0 },
			/*46*/Generator{ -1 },
			/*47*/Generator{ -1 },
			/*48*/Generator{ 0 },
			/*49*/Generator{  },
			/*50*/Generator{ 0 },
			/*51*/Generator{ 0 },
			/*52*/Generator{ 0 },
			/*53*/Generator{  },
			/*54*/Generator{ 0 },
			/*55*/Generator{  },
			/*56*/Generator{ 100 },
			/*57*/Generator{ 0 },
			/*58*/Generator{ -1 },
			/*59*/Generator{  },
			/*60*/Generator{  }
		};
		enum class GeneratorTypes
		{
			startAddrsOffset,
			endAddrsOffset,
			startloopAddrsOffset,
			endloopAddrsOffset,
			startAddrsCoarseOffset,
			modLfoToPitch,
			vibLfoToPitch,
			modEnvToPitch,
			initialFilterFc,
			initialFilterQ,
			modLfoToFilterFc,
			modEnvToFilterFc,
			endAddrsCoarseOffset,
			modLfoToVolume,
			unused1,
			chorusEffectsSend,
			reverbEffectsSend,
			pan,
			unused2,
			unused3,
			unused4,
			delayModLfo,
			freqModLfo,
			delayVibLfo,
			freqVibLfo,
			delayModEnv,
			attackModEnv,
			holdModEnv,
			decayModEnv,
			sustainModEnv,
			releaseModEnv,
			keynumToModEnvHold,
			keynumToModEnvDecay,
			delayVolEnv,
			attackVolEnv,
			holdVolEnv,
			decayVolEnv,
			sustainVolEnv,
			releaseVolEnv,
			keynumToVolEnvHold,
			keynumToVolEnvDecay,
			instrument,
			reserved1,
			keyRange,
			velRange,
			startloopAddrsCoarseOffset,
			keynum,
			velocity,
			initialAttenuation,
			reserved2,
			endloopAddrsCoarseOffset,
			coarseTune,
			fineTune,
			sampleID,
			sampleModes,
			reserved3,
			scaleTuning,
			exclusiveClass,
			overridingRootKey,
			unused5,
			endOper
		};
		Bag()
		{

		}

		Bag(const unsigned int index,
			const std::vector<Bank::PresetDataChunk::Bag>& bagList,
			const std::vector<Bank::PresetDataChunk::Generator>& generatorList,
			const std::vector<Modulator>& modulatorList)
		{
			for (unsigned int generatorIndex{ bagList[index].generatorIndex }; generatorIndex < bagList[index + 1].generatorIndex; ++generatorIndex)
			{
				const Bank::PresetDataChunk::Generator& generator{ generatorList[generatorIndex] };
				generators[generator.generator].set(generator);
			}
			for (unsigned int modulatorIndex{ bagList[index].modulatorIndex }; modulatorIndex < bagList[index + 1].modulatorIndex; ++modulatorIndex)
			{

			}
		}
	};

	struct Preset
	{
	private:
		bool containsGlobalZone{ false };
	public:
		std::string name;
		Word presetNumber;
		Word bankNumber;
		Dword library, genre, morphology;
		std::vector<Bag> bags;
		std::array<std::array<std::vector<unsigned int>, 128>, 128> opt;
		Bag globalZone;
		bool hasGlobalZone() const
		{
			return this->containsGlobalZone;
		}

		Preset(const unsigned int index,
			const Bank::PresetDataChunk& presetDataChunk)
			:
			name{ presetDataChunk.presetHeaderChunk.presetHeaders[index].name },
			presetNumber{ presetDataChunk.presetHeaderChunk.presetHeaders[index].presetNumber },
			bankNumber{ presetDataChunk.presetHeaderChunk.presetHeaders[index].bankNumber },
			library{ presetDataChunk.presetHeaderChunk.presetHeaders[index].library },
			genre{ presetDataChunk.presetHeaderChunk.presetHeaders[index].genre },
			morphology{ presetDataChunk.presetHeaderChunk.presetHeaders[index].morphology }
		{
			for (unsigned int bagIndex{ presetDataChunk.presetHeaderChunk.presetHeaders[index].bagIndex }; bagIndex < presetDataChunk.presetHeaderChunk.presetHeaders[index+1].bagIndex; ++bagIndex)
			{
				bags.emplace_back(
					bagIndex,
					presetDataChunk.presetBagChunk.bags,
					presetDataChunk.presetGeneratorChunk.generators,
					presetDataChunk.presetModulatorChunk.modulators);
			}

			if (!bags[0].generators[(unsigned int)(Bag::GeneratorTypes::instrument)].isSet)
			{
				globalZone = bags[0];
				bags.erase(bags.begin());
				this->containsGlobalZone = true;
			}

			if (this->hasGlobalZone())
			{
				for (auto bag{ bags.begin() }; bag < bags.end(); ++bag)
				{
					for (unsigned int generator{ 0 }; generator < Bag::generatorCount; ++generator)
					{
						if (!bag->generators[generator].isSet)
						{
							if (this->globalZone.generators[generator].isSet)
							{
								bag->generators[generator].set(this->globalZone.generators[generator]);
							}
						}
					}
				}
			}
			for (auto bag{ bags.begin() }; bag < bags.end(); ++bag)
			{
				for (unsigned int key{ bag->generators[(unsigned int)(Bag::GeneratorTypes::keyRange)].amount.range.low }; key <= bag->generators[(unsigned int)(Bag::GeneratorTypes::keyRange)].amount.range.high; ++key)
				{
					for (unsigned int velocity{ bag->generators[(unsigned int)(Bag::GeneratorTypes::velRange)].amount.range.low }; velocity <= bag->generators[(unsigned int)(Bag::GeneratorTypes::velRange)].amount.range.high; ++velocity)
					{
						opt[key][velocity].push_back(bag - bags.begin());
					}
				}
			}
		}
		Preset()
		{

		}
	};

	struct Instrument
	{
	private:
		bool containsGlobalZone{ false };
	public:
		std::string name;
		std::vector<Bag> bags;
		std::array<std::array<std::vector<unsigned int>, 128>, 128> opt;
		Bag globalZone;
		bool hasGlobalZone() const
		{
			return this->containsGlobalZone;
		}

		Instrument(
			const unsigned int index,
			const Bank::PresetDataChunk& presetDataChunk)
			:name{ presetDataChunk.instrumentChunk.instruments[index].name }
		{
			for (unsigned int bagIndex{ presetDataChunk.instrumentChunk.instruments[index].bagIndex }; bagIndex < presetDataChunk.instrumentChunk.instruments[index+1].bagIndex; ++bagIndex)
			{
				bags.emplace_back(
					bagIndex,
					presetDataChunk.instrumentBagChunk.bags,
					presetDataChunk.instrumentGeneratorChunk.generators,
					presetDataChunk.instrumentModulatorChunk.modulators);
			}

			if (!bags[0].generators[(unsigned int)(Bag::GeneratorTypes::sampleID)].isSet)
			{
				globalZone = bags[0];
				bags.erase(bags.begin());
				this->containsGlobalZone = true;
			}

			if (this->hasGlobalZone())
			{
				for (auto bag{ bags.begin() }; bag < bags.end(); ++bag)
				{
					for (unsigned int generator{ 0 }; generator < Bag::generatorCount; ++generator)
					{
						if (!bag->generators[generator].isSet)
						{
							if (this->globalZone.generators[generator].isSet)
							{
								bag->generators[generator].set(this->globalZone.generators[generator]);
							}
						}
					}
				}
			}
			for (auto bag{ bags.begin() }; bag < bags.end(); ++bag)
			{
				for (unsigned int key{ bag->generators[(unsigned int)(Bag::GeneratorTypes::keyRange)].amount.range.low }; key <= bag->generators[(unsigned int)(Bag::GeneratorTypes::keyRange)].amount.range.high; ++key)
				{
					for (unsigned int velocity{ bag->generators[(unsigned int)(Bag::GeneratorTypes::velRange)].amount.range.low }; velocity <= bag->generators[(unsigned int)(Bag::GeneratorTypes::velRange)].amount.range.high; ++velocity)
					{
						opt[key][velocity].push_back(bag - bags.begin());
					}
				}
			}
		}
		Instrument()
		{

		}
	};

	std::map<std::pair<unsigned int, unsigned int>, Preset> presets;//TODO: this map would be better as an unordered_map, but a hashing function for pairs is needed. So make one. Also don't forget to remove the <map> include
	std::vector<Instrument> instruments;
	using SampleHeader = Bank::PresetDataChunk::SampleHeaderChunk::SampleHeader;
	std::vector<SampleHeader> samples;
	SoundfontFile(const std::string_view& data)
	{
		SoundfontStream dataStream{ data };
		Bank bank{ dataStream.popRiff() };

		version = bank.infoChunk.versionChunk;
		name = bank.infoChunk.nameChunk.name;
		targetEngine = bank.infoChunk.targetEngineChunk.targetEngine;

		sampleDataPoints = bank.sampleDataChunk.sampleDataPoints;

		
		for (unsigned int presetIndex{ 0 }; presetIndex < bank.presetDataChunk.presetHeaderChunk.presetHeaders.size() - 1; ++presetIndex)
		{
			const Preset newPreset{ presetIndex, bank.presetDataChunk };
			presets.emplace(std::make_pair(newPreset.bankNumber, newPreset.presetNumber), newPreset);
		}

		for (unsigned int instrumentIndex{ 0 }; instrumentIndex < bank.presetDataChunk.instrumentChunk.instruments.size() - 1; ++instrumentIndex)
		{
			instruments.emplace_back(instrumentIndex, bank.presetDataChunk);
		}
		samples = bank.presetDataChunk.sampleHeaderChunk.samples;
		if (!dataStream.isEmpty())
			throw 0;
	}
	SoundfontFile()
	{

	}
};
#endif
