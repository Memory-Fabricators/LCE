//! Embedder-facing API for driving named AVM2 display objects by property/method name.

use ruffle_core::avm2::activation::Activation as Avm2Activation;
use ruffle_core::avm2::{FunctionArgs, Value as Avm2Value};
use ruffle_core::context::UpdateContext;
use ruffle_core::display_object::{TDisplayObject, TDisplayObjectContainer};
use ruffle_core::external::Value as ExternalValue;
use ruffle_core::string::{from_utf8, AvmString};
use std::fmt;

#[derive(Debug)]
pub enum NamedObjectError {
    /// No direct child of the stage's root display object has this instance name.
    NotFound,
    /// The child exists but isn't an AVM2 stage object (e.g. AVM1 content,
    /// or a non-scriptable display object like a plain `Bitmap`).
    NotScriptable,
    /// The AS call raised/threw, or the property/method doesn't exist on the object's class.
    ScriptError(String),
}

impl fmt::Display for NamedObjectError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            NamedObjectError::NotFound => write!(f, "no such named child on the display list"),
            NamedObjectError::NotScriptable => write!(f, "child is not an AVM2 scriptable object"),
            NamedObjectError::ScriptError(e) => write!(f, "AS error: {e}"),
        }
    }
}

fn resolve_named_child<'gc>(
    context: &mut UpdateContext<'gc>,
    instance_name: &str,
) -> Result<Avm2Value<'gc>, NamedObjectError> {
    let root = context.stage.root_clip().ok_or(NamedObjectError::NotFound)?;
    let container = root.as_container().ok_or(NamedObjectError::NotFound)?;
    let name = from_utf8(instance_name);
    let child = container
        .child_by_name(&name, false)
        .ok_or(NamedObjectError::NotFound)?;
    let stage_object = child.object2().ok_or(NamedObjectError::NotScriptable)?;
    Ok(Avm2Value::Object(stage_object.into()))
}

fn to_avm2_value<'gc>(value: &ExternalValue, activation: &mut Avm2Activation<'_, 'gc>) -> Avm2Value<'gc> {
    match value {
        ExternalValue::Undefined => Avm2Value::Undefined,
        ExternalValue::Null => Avm2Value::Null,
        ExternalValue::Bool(b) => Avm2Value::from(*b),
        ExternalValue::Number(n) => Avm2Value::from(*n),
        ExternalValue::String(s) => Avm2Value::from(AvmString::new_utf8(activation.gc(), s.as_str())),
        ExternalValue::Object(_) | ExternalValue::List(_) => Avm2Value::Undefined,
    }
}

fn from_avm2_value<'gc>(value: Avm2Value<'gc>, activation: &mut Avm2Activation<'_, 'gc>) -> ExternalValue {
    match value {
        Avm2Value::Undefined => ExternalValue::Undefined,
        Avm2Value::Null => ExternalValue::Null,
        Avm2Value::Bool(b) => ExternalValue::Bool(b),
        Avm2Value::Number(n) => ExternalValue::Number(n),
        Avm2Value::Integer(i) => ExternalValue::Number(i as f64),
        Avm2Value::String(s) => ExternalValue::String(s.to_string()),
        Avm2Value::Object(_) => match value.coerce_to_string(activation) {
            Ok(s) => ExternalValue::String(s.to_string()),
            Err(_) => ExternalValue::Undefined,
        },
    }
}

/// Calls a public method by name on a named child of the stage's root
/// display object, translating arguments and the return value through the
/// embedder-safe `external::Value` format.
pub fn call_named_child_method<'gc>(
    context: &mut UpdateContext<'gc>,
    instance_name: &str,
    method_name: &str,
    args: &[ExternalValue],
) -> Result<ExternalValue, NamedObjectError> {
    let object = resolve_named_child(context, instance_name)?;
    let mut activation = Avm2Activation::from_nothing(context);
    let avm2_args: Vec<Avm2Value> = args.iter().map(|a| to_avm2_value(a, &mut activation)).collect();
    let method = AvmString::new_utf8(activation.gc(), method_name);
    let result = object
        .call_public_property(method, FunctionArgs::from_slice(&avm2_args), &mut activation)
        .map_err(|e| NamedObjectError::ScriptError(format!("{e:?}")))?;
    Ok(from_avm2_value(result, &mut activation))
}
