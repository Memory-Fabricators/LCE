//! Owns a Ruffle `Player` bound to an `OpenGlRenderer` and drives its
//! per-frame tick/render cycle plus mouse input forwarding.

use crate::navigator::SearchPathNavigatorBackend;
use crate::opengl_renderer::OpenGlRenderer;
use ruffle_core::backend::navigator::NullExecutor;
use ruffle_core::events::MouseButton as RuffleMouseButton;
use ruffle_core::tag_utils::SwfMovie;
use ruffle_core::{FloatDuration, Player, PlayerBuilder, PlayerEvent, ViewportDimensions};
use std::path::PathBuf;
use std::sync::{Arc, Mutex};

pub struct BridgePlayer {
    player: Arc<Mutex<Player>>,
    // Drives futures spawned by the navigator (e.g. companion-SWF fetches
    // triggered by ImportAssets/ImportAssets2 tags in legacy Iggy-authored
    // menu movies, which reference shared skin/graphics libraries by a
    // relative URL resolved against `search_dirs`).
    executor: Mutex<NullExecutor>,
}

impl BridgePlayer {
    pub fn new(
        width: u32,
        height: u32,
        swf_data: &[u8],
        url: String,
        search_dirs: Vec<PathBuf>,
    ) -> Result<Self, ()> {
        let renderer = OpenGlRenderer::new(width, height)?;
        let movie = SwfMovie::from_data(swf_data, url, None, None).map_err(|_| ())?;
        let executor = NullExecutor::new();
        let navigator = SearchPathNavigatorBackend::new(search_dirs, &executor);
        let player = PlayerBuilder::new()
            .with_renderer(renderer)
            .with_navigator(navigator)
            .with_viewport_dimensions(width, height, 1.0)
            .with_movie(movie)
            .with_autoplay(true)
            .build();
        let bridge_player = Self {
            player,
            executor: Mutex::new(executor),
        };
        // `tick()` only advances the timeline once `frame_accumulator` covers a
        // full frame duration; the accumulator starts at zero, so without this
        // the first real per-host-frame `tick()` call (with a tiny elapsed dt)
        // would leave the display list empty and `render()` would draw nothing
        // but the background color. Force several frames' worth of time,
        // pumping the navigator executor between each, so that: (a) frame 1's
        // PlaceObject/AVM logic runs, and (b) any ImportAssets-driven companion
        // SWF fetch (which halts the importing clip's own preload until the
        // companion resolves) has enough preload ticks to complete before the
        // caller's first `render()`.
        for _ in 0..60 {
            bridge_player.pump_navigator();
            bridge_player
                .player
                .lock()
                .unwrap()
                .tick(FloatDuration::from_secs(1.0));
        }
        bridge_player.pump_navigator();
        Ok(bridge_player)
    }

    fn pump_navigator(&self) {
        self.executor.lock().unwrap().run();
    }

    /// Runs `f` with a live `UpdateContext` for this player. Used by
    /// higher-level bridges (e.g. `menu_button`) that need direct AVM2
    /// access beyond the fixed operations below.
    pub fn with_update_context<F, R>(&self, f: F) -> R
    where
        F: for<'gc> FnOnce(&mut ruffle_core::context::UpdateContext<'gc>) -> R,
    {
        self.player.lock().unwrap().mutate_with_update_context(f)
    }

    pub fn resize(&self, width: u32, height: u32) {
        if width == 0 || height == 0 {
            return;
        }
        self.player
            .lock()
            .unwrap()
            .set_viewport_dimensions(ViewportDimensions {
                width,
                height,
                scale_factor: 1.0,
            });
    }

    pub fn tick(&self, dt_seconds: f64) {
        self.pump_navigator();
        self.player
            .lock()
            .unwrap()
            .tick(FloatDuration::from_secs(dt_seconds));
        self.pump_navigator();
    }

    pub fn render(&self) {
        self.player.lock().unwrap().render();
    }

    pub fn handle_mouse_move(&self, x: f64, y: f64) {
        self.player
            .lock()
            .unwrap()
            .handle_event(PlayerEvent::MouseMove { x, y });
    }

    pub fn handle_mouse_button(&self, x: f64, y: f64, button: RuffleMouseButton, down: bool) {
        let mut player = self.player.lock().unwrap();
        if down {
            player.handle_event(PlayerEvent::MouseDown {
                x,
                y,
                button,
                index: None,
            });
        } else {
            player.handle_event(PlayerEvent::MouseUp { x, y, button });
        }
    }

    pub fn handle_mouse_leave(&self) {
        self.player
            .lock()
            .unwrap()
            .handle_event(PlayerEvent::MouseLeave);
    }
}
