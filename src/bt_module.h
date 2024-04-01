#ifndef SRC_BT_MODULE_H_
#define SRC_BT_MODULE_H_

#include <drivers/wireless/rn_52.h>

class BtModule {
public:
    BtModule(Libp::Rn52& rn52) : rn52_(rn52) { }

    static constexpr char dev_name[] = "Bath One";
    // 0x200414 : audio->audio/video->loudspeaker
    // 0x240414 : audio/rendering->audio/video->loudspeaker
    static constexpr char cod[] = { '2','0','0','4','1','4' };

    bool init()
    {
        bool success =
                rn52_.setDevName(dev_name)
             && rn52_.setCod(cod)
             && rn52_.setDiscoveryMask<Libp::Rn52::Profile::a2dp>()
             && rn52_.setAudioOut(Libp::Rn52::AudioOutput::analog, Libp::Rn52::BitsPerSample::bits_24, Libp::Rn52::SampleRate::rate_44k)
             && rn52_.setConnectionMask<Libp::Rn52::Profile::a2dp>()
             && rn52_.setAuth(Libp::Rn52::AuthMethod::ssp_keyboard)
             && rn52_.setFeatures<Libp::Rn52::Feature::en_track_change_event>()
             && rn52_.actReboot();
        return success;
    }
    uint16_t queryStatus()
    {
        return rn52_.actQueryStatus();
    }
    bool reboot()
    {
        return rn52_.actReboot();
    }

    bool getMetadata(Libp::Rn52::MetaData* metadata)
    {
        return rn52_.avrcpGetMetaData(metadata);
    }

    bool enterPairingMode()
    {
        return rn52_.basicActionCmd(Libp::Rn52::Cmd::dicoverable_on);
    }
    bool exitPairingMode()
    {
        return rn52_.basicActionCmd(Libp::Rn52::Cmd::dicoverable_off);
    }
    bool awaitPairingPasskey(char (&passkey)[6])
    {
        // Animation frames are ~80ms apart so wait for 50ms
        return rn52_.awaitPairingPasskey(passkey, 50);
    }
    bool acceptPairing()
    {
        return rn52_.basicActionCmd(Libp::Rn52::Cmd::accept_pairing);
    }
    bool resetPairings()
    {
        return rn52_.basicActionCmd(Libp::Rn52::Cmd::clear_saved_pairings);
    }
    void volUp()
    {
        rn52_.basicActionCmd(Libp::Rn52::Cmd::avrcp_vol_up);
    }
    void volDown()
    {
        rn52_.basicActionCmd(Libp::Rn52::Cmd::avrcp_vol_down);
    }
    void trackNext()
    {
        rn52_.basicActionCmd(Libp::Rn52::Cmd::avrcp_next);
    }
    void trackPrev()
    {
        rn52_.basicActionCmd(Libp::Rn52::Cmd::avrcp_prev);
    }
    void playPause()
    {
        rn52_.basicActionCmd(Libp::Rn52::Cmd::avrcp_play_pause);
    }
private:
    Libp::Rn52& rn52_;
};

#endif /* SRC_BT_MODULE_H_ */
