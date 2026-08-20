package pt.netpack.xfb.companion

import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Handler
import android.os.Looper
import android.util.Log
import androidx.media3.common.AudioAttributes
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.MediaMetadata
import androidx.media3.common.PlaybackException
import androidx.media3.common.PlaybackParameters
import androidx.media3.common.Player
import androidx.media3.exoplayer.DefaultRenderersFactory
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.RenderersFactory
import androidx.media3.exoplayer.audio.AudioSink
import androidx.media3.exoplayer.audio.DefaultAudioSink
import androidx.media3.exoplayer.source.ShuffleOrder
import androidx.media3.session.MediaSession
import androidx.media3.session.MediaSessionService
import kotlin.math.max

/**
 * Plays a downloaded playlist, offline, with XFB's crossfades.
 *
 * Two decks, as on the desktop. Both hold the whole playlist; one is audible and
 * attached to the session, the other waits on the next track, prepared and
 * silent. A crossfade starts the waiting deck and fades the playing one out,
 * then the two swap roles.
 *
 * The earlier design handed the *outgoing* track from one player to the other at
 * the crossfade point, which cannot be made seamless: a prepared-but-paused
 * player needs about 150 ms to actually emit audio, while one that is already
 * streaming sits about 150 ms behind where it was started — and correcting that
 * by seeking costs another 120 ms. So the outgoing track either fell silent for
 * a moment or repeated a slice of itself. Recorded off the phone's speaker, that
 * was a 60 ms hole at every join. Here nothing is ever handed over: the outgoing
 * track simply plays on, fading, and the incoming one starts on the other deck.
 */
class PlaybackService : MediaSessionService() {

    private var deckA: ExoPlayer? = null
    private var deckB: ExoPlayer? = null

    /** The audible deck, and the one the session is attached to. */
    private var current: ExoPlayer? = null
    /** Prepared and silent, waiting on the next track. */
    private var standby: ExoPlayer? = null

    private var session: MediaSession? = null

    private val ticker = Handler(Looper.getMainLooper())
    private var tracks: List<SavedTrack> = emptyList()
    private var envelopes: List<List<Pair<Long, Float>>> = emptyList()
    private var loadedPlaylist: String? = null

    /** Guards against firing the same crossfade twice as the ticker runs. */
    private var segueFiredForIndex = -1

    /** The deck on its way out, if a crossfade is under way. */
    private var fadingDeck: ExoPlayer? = null
    private var fadeStartedAt = 0L
    private var fadeDurationMs = 0L
    private var fadeFromGain = 1f

    private var lastLogAt = 0L

    /**
     * How many tracks in a row have failed to decode. Bounds the walk forward
     * so a set of entirely unreadable files stops instead of spinning.
     */
    private var unplayableRun = 0

    /** Shuffle, and the seed both decks share so they agree on the order. */
    private var shuffle = false
    // Drawn per process rather than left at zero: shuffle remembered from a
    // previous run would otherwise replay the very same order every launch.
    private var shuffleSeed = System.nanoTime()

    /** XFB's equalizer and compressor, one instance per deck. */
    private var fxA: FxAudioProcessor? = null
    private var fxB: FxAudioProcessor? = null

    override fun onCreate() {
        super.onCreate()

        val attributes = AudioAttributes.Builder()
            .setContentType(C.AUDIO_CONTENT_TYPE_MUSIC)
            .setUsage(C.USAGE_MEDIA)
            .build()

        // A chain each: the decks are audible together across a crossfade, so a
        // shared one would have both writing through the same filter state.
        val settings = FxSettingsStore(this).load()
        fxA = FxAudioProcessor(settings)
        fxB = FxAudioProcessor(settings)

        // Only one deck handles audio focus; two players both reacting to it
        // would fight each other over ducking.
        val a = ExoPlayer.Builder(this)
            .setRenderersFactory(fxRenderers(fxA!!))
            .setAudioAttributes(attributes, true)
            .setHandleAudioBecomingNoisy(true)
            .build()
        val b = ExoPlayer.Builder(this)
            .setRenderersFactory(fxRenderers(fxB!!))
            .setAudioAttributes(attributes, false)
            .build()

        deckA = a
        deckB = b
        current = a
        standby = b

        applyFx(settings)
        shuffle = getSharedPreferences(PREFS, MODE_PRIVATE).getBoolean(KEY_SHUFFLE, false)

        a.addListener(deckListener(a))
        b.addListener(deckListener(b))

        val openPlayer = PendingIntent.getActivity(
            this, 0,
            Intent(this, PlayerActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        session = MediaSession.Builder(this, a)
            .setSessionActivity(openPlayer)
            .setCallback(object : MediaSession.Callback {})
            .build()

        ticker.post(tick)
    }

    /**
     * A renderers factory whose audio sink runs [fx] over the PCM on its way
     * out. Everything else is ExoPlayer's default — including the Sonic
     * processor the 432 Hz retune leans on, which the sink keeps at the end of
     * the chain behind whatever is passed here.
     */
    private fun fxRenderers(fx: FxAudioProcessor): RenderersFactory =
        object : DefaultRenderersFactory(this) {
            override fun buildAudioSink(
                context: Context,
                enableFloatOutput: Boolean,
                enableAudioTrackPlaybackParams: Boolean,
            ): AudioSink =
                DefaultAudioSink.Builder(context)
                    .setAudioProcessors(arrayOf(fx))
                    .build()
        }

    /**
     * Applies FX settings to both decks: the equalizer and compressor through
     * the native chain, and the 432 Hz retune as a pitch shift.
     */
    fun applyFx(settings: FxSettings) {
        fxA?.updateSettings(settings)
        fxB?.updateSettings(settings)

        // Pitch alone, speed left at 1 — the track must still last as long as
        // the crossfade arithmetic thinks it does. This is exactly what the
        // desktop's "asetrate then atempo" pair adds up to.
        val pitch = if (settings.retune432) FxSettings.RETUNE_432_PITCH else 1f
        val params = PlaybackParameters(1f, pitch)
        deckA?.playbackParameters = params
        deckB?.playbackParameters = params

        Log.d(TAG, "fx: retune=${settings.retune432} pitch=${deckA?.playbackParameters?.pitch} " +
                   "speed=${deckA?.playbackParameters?.speed}")
    }

    /** Re-parks the waiting deck whenever the audible one changes track. */
    private fun deckListener(deck: ExoPlayer) = object : Player.Listener {
        override fun onMediaItemTransition(mediaItem: MediaItem?, reason: Int) {
            if (deck !== current) return
            segueFiredForIndex = -1
            // Not while the other deck is still fading — it is about to become
            // the standby itself, and parking it would cut the fade short.
            if (fadingDeck == null) parkStandby()
        }

        override fun onIsPlayingChanged(isPlaying: Boolean) {
            // Something decoded, so whatever run of bad files came before it is
            // over.
            if (isPlaying) unplayableRun = 0
        }

        /**
         * A file that will not decode costs its own track, not the whole set.
         *
         * ExoPlayer stops on a source error, which is right for a playlist
         * somebody built and wrong for "everything on this phone" — that is an
         * arbitrary pile, and one stale file in it would look like the app
         * simply refusing to play. Stepping to the next item re-walks the same
         * order, shuffled or not; the run counter stops it looping when the
         * whole set is unreadable.
         */
        override fun onPlayerError(error: PlaybackException) {
            Log.w(TAG, "unplayable track at ${deck.currentMediaItemIndex}: ${error.errorCodeName}")

            if (++unplayableRun > tracks.size) {
                Log.w(TAG, "nothing in this set could be decoded; stopping")
                return
            }

            val next = nextIndexOf(deck)
            if (next !in tracks.indices) return

            val wasAudible = deck === current
            deck.seekTo(next, 0L)
            deck.prepare()
            if (wasAudible) deck.play() else deck.playWhenReady = false
        }
    }

    override fun onGetSession(controllerInfo: MediaSession.ControllerInfo): MediaSession? = session

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_RELOAD_MIX) {
            reloadMix()
            return super.onStartCommand(intent, flags, startId)
        }

        if (intent?.action == ACTION_RELOAD_FX) {
            applyFx(FxSettingsStore(this).load())
            return super.onStartCommand(intent, flags, startId)
        }

        if (intent?.action == ACTION_SHUFFLE) {
            setShuffle(intent.getBooleanExtra(EXTRA_SHUFFLE, false), reorder = true)
            return super.onStartCommand(intent, flags, startId)
        }

        val name = intent?.getStringExtra(EXTRA_PLAYLIST)
        if (name != null) {
            val index = intent.getIntExtra(EXTRA_INDEX, 0)
            resolvePlaylist(name)?.let { playlist ->
                load(playlist, index)
                current?.play()
            }
        }
        return super.onStartCommand(intent, flags, startId)
    }

    /**
     * The saved playlist of that name, or — for [ALL_TRACKS] — everything
     * sitting on this phone gathered from every manifest.
     *
     * The gathered set carries no overlaps: a crossfade belongs to a playlist
     * somebody shaped, and inventing one for an arbitrary pile of tracks would
     * be putting words in the operator's mouth. They play as clean cuts.
     */
    private fun resolvePlaylist(name: String): SavedPlaylist? {
        val library = LibraryStore(this)
        if (name != ALL_TRACKS) return library.loadManifest(name)

        val everything = library.gatheredSet()
        return if (everything.isEmpty()) null else SavedPlaylist(ALL_TRACKS, everything)
    }

    /** Picks up crossfades edited while the set is playing. */
    private fun reloadMix() {
        val playlistName = loadedPlaylist ?: return
        // Through the same resolver as loading, so a crossfade set on the
        // gathered set is picked up exactly like one set on a playlist.
        val saved = resolvePlaylist(playlistName) ?: return

        val byId = saved.tracks.associateBy { it.id }
        tracks = tracks.map { track ->
            byId[track.id]?.let {
                track.copy(overlapMs = it.overlapMs, volumeEnvelope = it.volumeEnvelope)
            } ?: track
        }
        envelopes = tracks.map { VolumeEnvelope.parse(it.volumeEnvelope) }
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
        val playing = current
        if (playing == null || !playing.playWhenReady || playing.mediaItemCount == 0) stopSelf()
    }

    override fun onDestroy() {
        ticker.removeCallbacksAndMessages(null)
        session?.release()
        session = null
        deckA?.release(); deckA = null
        deckB?.release(); deckB = null
        current = null
        standby = null
        // Only once the players are gone: release() joins the playback thread,
        // so after it nothing can still be inside the native chain.
        fxA?.release(); fxA = null
        fxB?.release(); fxB = null
        super.onDestroy()
    }

    // ---------------------------------------------------------------- queue

    fun load(playlist: SavedPlaylist, startIndex: Int) {
        val playing = current ?: return
        val waiting = standby ?: return

        loadedPlaylist = playlist.name
        tracks = playlist.tracks.filter { it.playable }
        envelopes = tracks.map { VolumeEnvelope.parse(it.volumeEnvelope) }

        val displayName = if (playlist.name == ALL_TRACKS)
            getString(R.string.player_all_tracks) else playlist.name

        val items = tracks.map { track ->
            MediaItem.Builder()
                .setUri(track.file.toURI().toString())
                .setMediaId(track.id)
                .setMediaMetadata(
                    MediaMetadata.Builder()
                        .setTitle(track.song)
                        .setArtist(track.artist.ifEmpty { displayName })
                        // The raw name, because this is what identifies the set
                        // to the player screen when it reattaches.
                        .setAlbumTitle(playlist.name)
                        .build()
                )
                .build()
        }

        // Both decks carry the whole playlist, so whichever one is audible gives
        // the session a complete queue — next, previous and the track list all
        // keep working after a swap.
        val start = startIndex.coerceIn(0, max(0, items.size - 1))
        playing.setMediaItems(items, start, 0L)
        playing.prepare()
        waiting.setMediaItems(items, start, 0L)
        waiting.prepare()

        segueFiredForIndex = -1
        fadingDeck = null
        fadeDurationMs = 0
        unplayableRun = 0

        // The order is built for a queue length, so it has to be rebuilt now
        // the queue has one — but keep the sequence, this is the same session.
        setShuffle(shuffle, reorder = false)
        parkStandby()
    }

    /**
     * Turns shuffle on or off across both decks.
     *
     * The two decks must agree on what "next" is. Each ExoPlayer would
     * otherwise roll its own random order, and the deck taking over after a
     * crossfade would be holding a different sequence from the one that just
     * started — so both are given the same order, built from one seed.
     *
     * @param reorder true to draw a fresh order, so switching shuffle off and
     *        on again does not replay the same sequence.
     */
    private fun setShuffle(enabled: Boolean, reorder: Boolean) {
        if (reorder) shuffleSeed = System.nanoTime()
        shuffle = enabled

        // The order has to start on the track that is playing. A plain random
        // permutation puts it wherever it falls, and when that is the last
        // position there is no next track at all: the crossfade never fires and
        // the set stops at the end of whatever happened to be on. Shuffling
        // means shuffling what is still to come.
        val order = shuffledOrder(tracks.size, current?.currentMediaItemIndex ?: 0, shuffleSeed)

        for (deck in listOfNotNull(deckA, deckB)) {
            if (order.isNotEmpty()) {
                deck.setShuffleOrder(ShuffleOrder.DefaultShuffleOrder(order, shuffleSeed))
            }
            deck.shuffleModeEnabled = enabled
        }

        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putBoolean(KEY_SHUFFLE, enabled).apply()

        // The track after this one has just changed, so the parked deck is on
        // the wrong one until it is re-parked.
        segueFiredForIndex = -1
        if (fadingDeck == null) parkStandby()

        Log.d(TAG, "shuffle=$enabled seed=$shuffleSeed " +
                   "current=${current?.currentMediaItemIndex} next=${current?.nextMediaItemIndex}")
    }

    /**
     * A play order for [count] tracks that begins on [first], with everything
     * else after it in random order. Handed to both decks so they agree.
     */
    private fun shuffledOrder(count: Int, first: Int, seed: Long): IntArray {
        if (count <= 0) return IntArray(0)
        val head = first.coerceIn(0, count - 1)
        val rest = (0 until count).filter { it != head }.toMutableList()
        rest.shuffle(java.util.Random(seed))
        return (listOf(head) + rest).toIntArray()
    }

    /**
     * Where the audible deck will go next, honouring shuffle. Reading
     * `currentMediaItemIndex + 1` would walk the queue in its stored order and
     * quietly ignore the shuffle.
     */
    private fun nextIndexOf(player: ExoPlayer): Int {
        val next = player.nextMediaItemIndex
        return if (next == C.INDEX_UNSET) -1 else next
    }

    /** Positions the silent deck on the next track, ready to start at once. */
    private fun parkStandby() {
        val playing = current ?: return
        val waiting = standby ?: return

        val next = nextIndexOf(playing)
        if (next !in tracks.indices) return

        waiting.volume = 0f
        waiting.playWhenReady = false
        waiting.seekTo(next, 0L)
    }

    // -------------------------------------------------------------- the tick

    private val tick = object : Runnable {
        override fun run() {
            step()
            ticker.postDelayed(this, TICK_MS)
        }
    }

    private fun step() {
        applyFade()

        val playing = current ?: return
        if (!playing.isPlaying) return

        val index = playing.currentMediaItemIndex
        if (index !in tracks.indices) return

        playing.volume = VolumeEnvelope.gainAt(envelopes[index], playing.currentPosition)

        if (VERBOSE) {
            val now = System.currentTimeMillis()
            if (now - lastLogAt > 1000) {
                lastLogAt = now
                Log.d(TAG, "index=$index pos=${playing.currentPosition}ms vol=%.3f"
                    .format(playing.volume))
            }
        }

        val nextIndex = nextIndexOf(playing)
        if (nextIndex !in tracks.indices) return

        // In XFB an overlap belongs to the track being mixed *into*.
        val overlap = tracks[nextIndex].overlapMs
        if (overlap <= 0 || segueFiredForIndex == index) return

        val duration = playing.duration
        if (duration == C.TIME_UNSET || duration <= 0) return

        val untilSegue = (duration - overlap) - playing.currentPosition
        if (untilSegue > SCHEDULE_WITHIN_MS) return

        segueFiredForIndex = index
        Log.d(TAG, "segue: index=$index in ${untilSegue}ms, ${overlap}ms into " +
                   "'${tracks[nextIndex].song}'")
        ticker.postDelayed({ startSegue(overlap) }, untilSegue.coerceAtLeast(0))
    }

    /**
     * Starts the waiting deck on the next track and hands the session to it,
     * leaving the previous deck to fade out on its own.
     */
    private fun startSegue(overlapMs: Long) {
        val outgoing = current ?: return
        val incoming = standby ?: return

        val nextIndex = incoming.currentMediaItemIndex
        if (nextIndex !in tracks.indices) return

        Log.d(TAG, "crossfade: '${tracks[nextIndex].song}' starts on the other deck " +
                   "(state=${incoming.playbackState}, 3=ready)")

        incoming.volume = VolumeEnvelope.gainAt(envelopes[nextIndex], 0)
        incoming.play()

        // The outgoing track is never cut — only its level comes down — which
        // is why there is nothing here that can click.
        fadingDeck = outgoing
        fadeFromGain = outgoing.volume
        fadeStartedAt = System.currentTimeMillis()
        fadeDurationMs = overlapMs

        // Swap now, so the notification and lock screen name the incoming track
        // from the moment it can be heard.
        current = incoming
        standby = outgoing
        session?.player = incoming
        segueFiredForIndex = -1
    }

    private fun applyFade() {
        val fading = fadingDeck ?: return
        if (fadeDurationMs <= 0) return

        val elapsed = System.currentTimeMillis() - fadeStartedAt
        if (elapsed >= fadeDurationMs) {
            // Silent before pausing, so ending the deck cannot step the signal.
            fading.volume = 0f
            fading.pause()
            fadeDurationMs = 0
            fadingDeck = null
            // It is the waiting deck now, so put it on the track after next.
            parkStandby()
            return
        }

        fading.volume = fadeFromGain * (1f - elapsed.toFloat() / fadeDurationMs)
    }

    companion object {
        const val EXTRA_PLAYLIST = "playlist"
        const val EXTRA_INDEX = "index"
        const val ACTION_RELOAD_MIX = "pt.netpack.xfb.companion.RELOAD_MIX"
        const val ACTION_RELOAD_FX = "pt.netpack.xfb.companion.RELOAD_FX"
        const val ACTION_SHUFFLE = "pt.netpack.xfb.companion.SHUFFLE"
        const val EXTRA_SHUFFLE = "shuffle"

        /** Stands in for "everything downloaded onto this phone". */
        const val ALL_TRACKS = LibraryStore.ALL_TRACKS

        private const val PREFS = "playback"
        private const val KEY_SHUFFLE = "shuffle"
        private const val TAG = "XfbPlayback"
        private const val VERBOSE = false
        private const val TICK_MS = 40L
        /** Schedule the exact crossfade moment once this close to it. */
        private const val SCHEDULE_WITHIN_MS = 250L
    }
}
