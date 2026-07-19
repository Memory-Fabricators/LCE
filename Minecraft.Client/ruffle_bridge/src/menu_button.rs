//! Rust-side reimplementation of the small slice of the original (non-Ruffle)
//! Iggy `UIControl_Button`/`UIControl_Base` method-dispatch contract needed to
//! drive a single Flash menu button through Ruffle's AVM2, instead of
//! `IggyValuePathMakeNameRef`/`IggyPlayerCallMethodRS`/`registerFastName`.
//!
//! This is deliberately the *high-level* button API, not a generic
//! property/method proxy: C++ asks for "the button named X" and calls
//! `init`/`set_label`/`get_label`/`set_enabled` on it: the AS method names
//! (`Init`, `SetLabel`, `GetLabel`, `EnableButton` - see the reference-only,
//! non-compiled `Common/UI/UIControl_Button.cpp`/`UIControl_Base.cpp`) live
//! here in Rust, not in C++. It's built on `ruffle_core::embedder_ui`, the
//! narrow public name-path API added to Ruffle core for this bridge (see
//! `subprojects/packagefiles/ruffle/core/src/embedder_ui.rs`).
//!
//! A `MenuButtonHandle` is a *name-path* reference, re-resolved against the
//! live display list on every call - mirroring how the original
//! `IggyValuePath` name-reference worked - because AVM2 objects are
//! GC-arena-scoped and cannot outlive a single `mutate_with_update_context`
//! call, so nothing here can hold a live object handle between FFI calls.

use crate::player::BridgePlayer;
use crate::embedder_ui::{self, NamedObjectError};
use ruffle_core::external::Value as ExternalValue;

/// A name-path reference to a button instance, resolved as a direct child of
/// the root movie's display list.
pub struct MenuButtonHandle {
    instance_name: String,
}

pub type MenuButtonError = NamedObjectError;

impl MenuButtonHandle {
    pub fn new(instance_name: impl Into<String>) -> Self {
        Self {
            instance_name: instance_name.into(),
        }
    }

    fn call(
        &self,
        player: &BridgePlayer,
        method: &str,
        args: &[ExternalValue],
    ) -> Result<ExternalValue, MenuButtonError> {
        player.with_update_context(|context| {
            embedder_ui::call_named_child_method(context, &self.instance_name, method, args)
        })
    }

    /// Mirrors `UIControl_Button::init` -> AS `Init(label, id)`.
    pub fn init(&self, player: &BridgePlayer, label: &str, id: i32) -> Result<(), MenuButtonError> {
        self.call(
            player,
            "Init",
            &[
                ExternalValue::String(label.to_owned()),
                ExternalValue::Number(id as f64),
            ],
        )
        .map(|_| ())
    }

    /// Mirrors `UIControl_Base::setLabel` -> AS `SetLabel(label)`.
    pub fn set_label(&self, player: &BridgePlayer, label: &str) -> Result<(), MenuButtonError> {
        self.call(
            player,
            "SetLabel",
            &[ExternalValue::String(label.to_owned())],
        )
        .map(|_| ())
    }

    /// Mirrors `UIControl_Base::getLabel` -> AS `GetLabel()`.
    pub fn get_label(&self, player: &BridgePlayer) -> Result<String, MenuButtonError> {
        match self.call(player, "GetLabel", &[])? {
            ExternalValue::String(s) => Ok(s),
            _ => Ok(String::new()),
        }
    }

    /// Mirrors `UIControl_Button::EnableButton` -> AS `EnableButton(enable)`.
    pub fn set_enabled(&self, player: &BridgePlayer, enabled: bool) -> Result<(), MenuButtonError> {
        self.call(player, "EnableButton", &[ExternalValue::Bool(enabled)])
            .map(|_| ())
    }
}
