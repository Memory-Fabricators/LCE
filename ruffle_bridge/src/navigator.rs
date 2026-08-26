//! A minimal synchronous `file://` `NavigatorBackend` that resolves relative
//! asset URLs (e.g. `ImportAssets`-referenced companion SWFs) against a
//! search path of local directories, trying each in priority order.
//!
//! The legacy Iggy-authored menu SWFs this bridge loads were packed into a
//! single flat archive namespace (`MediaWindows64.arc`) built from several
//! source directories (a shared `Common/Media` tree plus a platform-specific
//! overlay). Reading loose files directly (bypassing the archive) needs the
//! same multi-directory search to find companion assets like platform skin
//! variants that only exist in the platform overlay directory.

use async_channel::{Receiver, Sender};
use indexmap::IndexMap;
use ruffle_core::backend::navigator::{
    ErrorResponse, NavigationMethod, NavigatorBackend, NullExecutor, NullSpawner, OwnedFuture,
    Request, SuccessResponse, fetch_path,
};
use ruffle_core::loader::Error;
use ruffle_core::socket::{ConnectionState, SocketAction, SocketHandle};
use std::path::{Path, PathBuf};
use std::time::Duration;
use url::{ParseError, Url};

pub struct SearchPathNavigatorBackend {
    spawner: NullSpawner,
    search_dirs: Vec<PathBuf>,
}

impl SearchPathNavigatorBackend {
    pub fn new(search_dirs: Vec<PathBuf>, executor: &NullExecutor) -> Self {
        Self {
            spawner: executor.spawner(),
            search_dirs,
        }
    }

    /// ImportAssets/companion URLs in these legacy assets are always bare
    /// relative filenames (e.g. `"skin.swf"`); search each candidate
    /// directory in priority order for a matching file. The original (non-Ruffle)
    /// Iggy engine registers its Windows64 platform-skin library into its own
    /// in-memory library table under the fixed logical name `"platformskin.swf"`
    /// (see `UIController.cpp`'s `loadSkin(platformSkinPath, L"platformskin.swf")`),
    /// even though the physical file loaded for it is named `skinWin.swf`/
    /// `skinHDWin.swf`. Ruffle's `ImportAssets` tag has no such indirection -
    /// it always requests the literal name baked into the SWF - so that one
    /// logical name is aliased to its real Windows64 HD filename here.
    fn resolve_local_path(&self, url: &str) -> Option<PathBuf> {
        let clean_name = url
            .trim_start_matches("file:///")
            .trim_start_matches("file://")
            .trim_start_matches('/');
        let real_name = if clean_name == "platformskin.swf" {
            "skinHDWin.swf"
        } else {
            clean_name
        };
        self.search_dirs
            .iter()
            .map(|dir| {
                if let Ok(canon) = dir.canonicalize() {
                    canon.join(real_name)
                } else {
                    dir.join(real_name)
                }
            })
            .find(|candidate| candidate.is_file())
    }

    fn first_search_dir(&self) -> PathBuf {
        self.search_dirs
            .first()
            .and_then(|d| d.canonicalize().ok())
            .or_else(|| self.search_dirs.first().cloned())
            .unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")))
    }
}
impl NavigatorBackend for SearchPathNavigatorBackend {
    fn navigate_to_url(
        &self,
        _url: &str,
        _target: &str,
        _vars_method: Option<(NavigationMethod, IndexMap<String, String>)>,
    ) {
    }

    fn fetch(&self, request: Request) -> OwnedFuture<Box<dyn SuccessResponse>, ErrorResponse> {
        fetch_path(self, "SearchPathNavigatorBackend", request.url(), None)
    }

    fn resolve_url(&self, url: &str) -> Result<Url, ParseError> {
        let path = self
            .resolve_local_path(url)
            .unwrap_or_else(|| self.first_search_dir().join(url));
        match Url::from_file_path(&path) {
            Ok(resolved) => Ok(self.pre_process_url(resolved)),
            // Not a `unix`/`windows`/`redox` target, or a malformed path;
            // fall back to plain URL parsing so the caller gets a normal
            // parse error rather than a panic.
            Err(()) => Url::parse(url),
        }
    }

    fn spawn_future(&mut self, future: OwnedFuture<(), Error>) {
        self.spawner.spawn_local(future);
    }

    fn pre_process_url(&self, url: Url) -> Url {
        url
    }

    fn connect_socket(
        &mut self,
        _host: String,
        _port: u16,
        _timeout: Duration,
        handle: SocketHandle,
        _receiver: Receiver<Vec<u8>>,
        sender: Sender<SocketAction>,
    ) {
        let _ = sender.try_send(SocketAction::Connect(handle, ConnectionState::Failed));
    }
}
