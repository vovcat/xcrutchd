// gcc -W -Wall -DMAIN -o aplaypop_switch aplaypop_switch.c -lasound && ./aplaypop_switch 1

#include <stdio.h>      // printf()
#include <unistd.h>     // write(), sleep()
#include <fcntl.h>      // O_WRONLY
#include <sys/ioctl.h>  // ioctl()

#include <alsa/asoundlib.h>

static char *card = "default"; //"plughw:0,0"; /* playback device */

// Sets or toggles Master mute switch: 0=mute, 1=unmute, -1=toggle

int aplaypop_switch(int value)
{
    int err = 0, smixer_level = 0;
    struct snd_mixer_selem_regopt smixer_options;
    snd_mixer_t *handle = NULL;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);

    smixer_options.ver = 1;
    smixer_options.abstract = SND_MIXER_SABSTRACT_NONE;
    smixer_options.abstract = SND_MIXER_SABSTRACT_BASIC;

    if ((err = snd_mixer_open(&handle, 0)) < 0) {
        printf("Mixer %s open error: %s\n", card, snd_strerror(err));
        return err;
    }

    if (smixer_level == 0 && (err = snd_mixer_attach(handle, card)) < 0) {
        printf("Mixer attach %s error: %s", card, snd_strerror(err));
        snd_mixer_close(handle);
        handle = NULL;
        return err;
    }
    if ((err = snd_mixer_selem_register(handle, smixer_level > 0 ? &smixer_options : NULL, NULL)) < 0) {
            printf("Mixer register error: %s", snd_strerror(err));
            snd_mixer_close(handle);
            handle = NULL;
            return err;
    }
    err = snd_mixer_load(handle);
    if (err < 0) {
            printf("Mixer %s load error: %s", card, snd_strerror(err));
            snd_mixer_close(handle);
            handle = NULL;
            return err;
    }

    snd_mixer_selem_id_set_name(sid, "Master");
    snd_mixer_selem_id_set_index(sid, 0);

    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
            printf("Unable to find simple control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
            snd_mixer_close(handle);
            handle = NULL;
            return -ENOENT;
    }

    if (snd_mixer_selem_is_enumerated(elem)) {
        // enum control
        printf("snd_mixer_selem_is_enumerated\n");
        //err = sset_enum(elem, argc, argv);
    } else {
        for (snd_mixer_selem_channel_id_t chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
            if (value == -1) {
                if ((err = snd_mixer_selem_get_playback_switch(elem, chn, &value)) < 0)
                    printf("snd_mixer_selem_get_playback_switch('%s',%i) = %d\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid), err);
                value = !value;
            } else {
                value = !!value;
            }

            if ((err = snd_mixer_selem_set_playback_switch(elem, chn, value)) < 0)
                printf("snd_mixer_selem_set_playback_switch('%s',%i) = %d\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid), err);

            snd_mixer_selem_get_playback_switch(elem, chn, &value);
            printf("Simple mixer control '%s',%i channel %d: value=%d selem_has_playback_switch_joined=%d\n", snd_mixer_selem_id_get_name(sid),
                snd_mixer_selem_id_get_index(sid), chn, value, snd_mixer_selem_has_playback_switch_joined(elem));

            if (snd_mixer_selem_has_playback_switch_joined(elem))
                break;
        }
    }

    snd_mixer_close(handle);
    return 0;
}


#ifdef MAIN
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc > 1 && argv[1][0] == '0') {
        aplaypop_switch(0); // mute
    } else if (argc > 1) {
        aplaypop_switch(1); // unmute
    } else {
        aplaypop_switch(-1); // toggle
    }
    return 0;
}
#endif
