#include "SFXBackend.h"

SFXSample::SFXSample(const SAMPLE &s)
{
    int datalen = (s.bits == 8 ? 1 : 2)*(s.stereo == 0 ? 1 : 2)*s.len;
    sample.data = new char[datalen];
    memcpy(sample.data, s.data, datalen);
    sample.bits = s.bits;
    sample.stereo = s.stereo;
    sample.freq = s.freq;
    sample.priority = s.priority;
    sample.len = s.len;
    sample.loop_start = s.loop_start;
    sample.loop_end = s.loop_end;
    sample.param = s.param;
    iscustom = true;
    name = std::string("Unnamed Sample");
}

SFXSample::~SFXSample()
{
    delete[] sample.data;
}

SFXBackend::SFXBackend() : volume_(248), pan_style_(1)
{
}

SFXBackend::~SFXBackend()
{
    clearSamples();
}

void SFXBackend::clearSamples()
{
    for (int i = 0; i < (int)voices_.size(); i++)
    {
        if (voices_[i].voice != -1)
            deallocate_voice(voices_[i].voice);
    }

    for (int i = 0; i < (int)samples_.size(); i++)
        if (samples_[i])
            delete samples_[i];

    samples_.clear();
    voices_.clear();
}

void SFXBackend::loadDefaultSamples(int numsamples, DATAFILE *sfxdata, const char * const*sfxnames)
{
    clearSamples();
    samples_.resize(numsamples);
    voices_.resize(numsamples);

    for (int i = 1; i<numsamples; i++)
    {
        loadDefaultSample(i, sfxdata, sfxnames);
        
    }
}

void SFXBackend::loadDefaultSample(int index, DATAFILE *sfxdata, const char * const* sfxnames)
{
    const char *oldname = sfxnames[index - 1];
    SAMPLE *temp_sample = (SAMPLE *)sfxdata[index].dat;

    SFXSample *sample = new SFXSample(*temp_sample);
    sample->iscustom = false;
    sample->name = std::string(oldname);
    samples_[index] = sample;
    voices_[index].voice = -1;
    voices_[index].loopCount = 0;
}

void SFXBackend::clearSample(int slot)
{    
    if (slot < 0 || slot >= (int)samples_.size())
        return;

    if (voices_[slot].voice != -1)
    {
        deallocate_voice(voices_[slot].voice);
    }

    if (samples_[slot])
        delete samples_[slot];

    samples_[slot] = NULL;
    voices_[slot].voice = -1;
    voices_[slot].loopCount = 0;
}

void SFXBackend::loadSample(int slot, const SAMPLE &sfx, const std::string name)
{
    if (slot < 0)
        return;

    if (slot < (int)voices_.size())
    {
        if (voices_[slot].voice != -1)
        {
            deallocate_voice(voices_[slot].voice);
        }
    }

    if (slot >= (int)samples_.size())
        samples_.resize(slot + 1);
    if (slot >= (int)voices_.size())
        voices_.resize(slot + 1);

    if (samples_[slot])
        delete samples_[slot];

    samples_[slot] = new SFXSample(sfx);
    samples_[slot]->iscustom = true;
    samples_[slot]->name = name;
    voices_[slot].voice = -1;
    voices_[slot].loopCount = 0;
}

const SFXSample *SFXBackend::getSample(int slot)
{
    if (slot < 0 || slot >= (int)samples_.size())
        return NULL;
    return samples_[slot];
}

int SFXBackend::numSlots()
{
    return samples_.size();
}

void SFXBackend::garbageCollect()
{
    for (int i = 0; i < (int)voices_.size(); i++)
    {
        if (voices_[i].voice != -1 && voices_[i].loopCount == 0 && voice_get_position(voices_[i].voice) < 0)
        {
            deallocate_voice(voices_[i].voice);
            voices_[i].voice = -1;
        }
    }
}

void SFXBackend::readConfigurationOptions(const std::string &prefix)
{
    std::string section = prefix + "-sfx";
    const char *secname = section.c_str();

    volume_ = get_config_int(secname, "volume", 248);
    pan_style_ = get_config_int(secname, "pan", 1);
}

void SFXBackend::writeConfigurationOptions(const std::string &prefix)
{
    std::string section = prefix + "-sfx";
    const char *secname = section.c_str();

    set_config_int(secname, "volume", volume_);
    set_config_int(secname, "pan", pan_style_);
}

void SFXBackend::setVolume(int volume)
{
    volume_ = volume;
    for (int i = 0; i < (int)voices_.size(); i++)
    {
        if (voices_[i].voice != -1)
        {
            voice_set_volume(voices_[i].voice, volume_);
        }
    }
}

int SFXBackend::allocateVoice(int slot)
{
    if (slot < 0 || slot >= (int)samples_.size() || slot >= (int)voices_.size() || !samples_[slot])
        return -1;

    if (voices_[slot].voice == -1)
    {
        voices_[slot].voice = allocate_voice(&samples_[slot]->sample);
    }
    if (voices_[slot].voice != -1)
    {
        voice_set_volume(voices_[slot].voice, volume_);
    }
    return voices_[slot].voice;
}

void SFXBackend::loop(int slot, int pan)
{
    pan = adjustPan(pan);
    int voice = allocateVoice(slot);
    if (voice == -1)
        return;

    if (voice_get_position(voice) <= 0)
    {
        // Sound is not currently playing. Start it in loop mode.
        voice_set_position(voice, 0);
        voice_set_playmode(voice, PLAYMODE_LOOP);
        voice_start(voice);        
    }
    else
    {
        voice_set_playmode(voice, PLAYMODE_LOOP);
    }
    voice_set_pan(voice, pan);
    voices_[slot].loopCount++;
}

void SFXBackend::play(int slot, int pan)
{
    pan = adjustPan(pan);
    int voice = allocateVoice(slot);
    if (voice == -1)
        return;

    voice_set_position(voice, 0);
    if (voices_[slot].loopCount > 0)
    {        
        return;
    }

    voice_set_pan(voice, pan);
    voice_set_playmode(voice, PLAYMODE_PLAY);
    voice_start(voice);
}
void SFXBackend::stop(int slot)
{
    if (slot < 0 || slot >= (int)voices_.size())
        return;

    int voice = voices_[slot].voice;
    if (voice != -1)
    {
        deallocate_voice(voice);
        voices_[slot].voice = -1;
        voices_[slot].loopCount = 0;
    }
}

void SFXBackend::stopAll()
{
    for (int i = 0; i < (int)voices_.size(); i++)
        stop(i);    
}

int SFXBackend::adjustPan(int pan)
{
    switch (pan_style_)
    {
    case 0:
        return 128;

    case 1:
        return std::min(std::max((pan >> 1) + 64, 0), 255);

    case 2:
        return std::min(std::max(((pan * 3) >> 2) + 32, 0), 255);

    case 3:
    default:
        return pan;
    }
}

void SFXBackend::pause(int slot)
{
    if (slot < 0 || slot >= (int)voices_.size())
        return;
    int voice = voices_[slot].voice;
    if(voice != -1)
        voice_stop(voice);
}

void SFXBackend::resume(int slot)
{
    if (slot < 0 || slot >= (int)voices_.size())
        return;
    int voice = voices_[slot].voice;
    if (voice != -1)
        voice_start(voice);
}

void SFXBackend::setIsCustom(int slot, bool iscustom)
{
    if (slot < 0 || slot >= (int)voices_.size())
        return;
    if (samples_[slot] == NULL)
        return;
    samples_[slot]->iscustom = iscustom;
}

void SFXBackend::unloop(int slot)
{
    if (slot < 0 || slot >= (int)voices_.size())
        return;
    if (voices_[slot].voice == -1 || voices_[slot].loopCount == 0)
        return;
    voices_[slot].loopCount--;
    if (voices_[slot].loopCount == 0)
    {
        voice_set_playmode(voices_[slot].voice, PLAYMODE_PLAY);
    }
}

bool SFXBackend::isPlaying(int slot)
{
    if (slot < 0 || slot >= (int)voices_.size())
        return false;
    if (voices_[slot].voice == -1)
        return false;
    if (voice_get_position(voices_[slot].voice) < 0)
        return false;
    return true;
}

void SFXBackend::pauseAll()
{
    for (int i = 0; i < (int)voices_.size(); i++)
        pause(i);
}

void SFXBackend::resumeAll()
{
    for (int i = 0; i < (int)voices_.size(); i++)
        resume(i);
}

int SFXBackend::getVolume()
{
    return volume_;
}

int SFXBackend::getPanStyle()
{
    return pan_style_;
}

void SFXBackend::setPanStyle(int style)
{
    if (style < 0 || style > 3)
        return;
    pan_style_ = style;
}
