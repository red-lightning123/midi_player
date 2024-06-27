struct VisualNote
{
public:
	unsigned int key, channel;
	unsigned int start, end;
	VisualNote(const unsigned int key, const unsigned int channel, const unsigned int start, const unsigned int end)
		:key(key), channel(channel), start(start), end(end)
	{

	}
};
