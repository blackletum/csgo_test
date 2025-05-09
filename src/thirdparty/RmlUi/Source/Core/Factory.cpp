﻿/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/ContextInstancer.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/EventListenerInstancer.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"

#include "../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "../../Include/RmlUi/Core/Elements/ElementFormControlDataSelect.h"
#include "../../Include/RmlUi/Core/Elements/ElementFormControlSelect.h"
#include "../../Include/RmlUi/Core/Elements/ElementFormControlSelect.h"
#include "../../Include/RmlUi/Core/Elements/ElementFormControlTextArea.h"
#include "../../Include/RmlUi/Core/Elements/ElementTabSet.h"
#include "../../Include/RmlUi/Core/Elements/ElementProgressBar.h"
#include "../../Include/RmlUi/Core/Elements/ElementDataGrid.h"
#include "../../Include/RmlUi/Core/Elements/ElementDataGridExpandButton.h"
#include "../../Include/RmlUi/Core/Elements/ElementDataGridCell.h"
#include "../../Include/RmlUi/Core/Elements/ElementDataGridRow.h"

#include "ContextInstancerDefault.h"
#include "DataControllerDefault.h"
#include "DataViewDefault.h"
#include "DecoratorTiledBoxInstancer.h"
#include "DecoratorTiledHorizontalInstancer.h"
#include "DecoratorTiledImageInstancer.h"
#include "DecoratorTiledVerticalInstancer.h"
#include "DecoratorNinePatch.h"
#include "DecoratorGradient.h"
#include "ElementHandle.h"
#include "ElementTextDefault.h"
#include "EventInstancerDefault.h"
#include "FontEffectBlur.h"
#include "FontEffectGlow.h"
#include "FontEffectOutline.h"
#include "FontEffectShadow.h"
#include "PluginRegistry.h"
#include "PropertyParserColour.h"
#include "StreamFile.h"
#include "StyleSheetFactory.h"
#include "TemplateCache.h"
#include "XMLNodeHandlerBody.h"
#include "XMLNodeHandlerDefault.h"
#include "XMLNodeHandlerHead.h"
#include "XMLNodeHandlerTemplate.h"
#include "XMLParseTools.h"

#include "Elements/ElementImage.h"
#include "Elements/ElementTextSelection.h"
#include "Elements/XMLNodeHandlerDataGrid.h"
#include "Elements/XMLNodeHandlerTabSet.h"
#include "Elements/XMLNodeHandlerTextArea.h"

#include <algorithm>

namespace Rml {

// Element instancers.
using ElementInstancerMap = UnorderedMap< String, ElementInstancer* >;
static ElementInstancerMap element_instancers;

// Decorator instancers.
using DecoratorInstancerMap = UnorderedMap< String, DecoratorInstancer* >;
static DecoratorInstancerMap decorator_instancers;

// Font effect instancers.
using FontEffectInstancerMap = UnorderedMap< String, FontEffectInstancer* >;
static FontEffectInstancerMap font_effect_instancers;

// Data view instancers.
using DataViewInstancerMap = UnorderedMap< String, DataViewInstancer* >;
static DataViewInstancerMap data_view_instancers;

// Data controller instancers.
using DataControllerInstancerMap = UnorderedMap< String, DataControllerInstancer* >;
static DataControllerInstancerMap data_controller_instancers;

// Structural data view instancers.
using StructuralDataViewInstancerMap = SmallUnorderedMap< String, DataViewInstancer* >;
static StructuralDataViewInstancerMap structural_data_view_instancers;

// Structural data view names.
static StringList structural_data_view_attribute_names;

// The context instancer.
static ContextInstancer* context_instancer = nullptr;

// The event instancer
static EventInstancer* event_instancer = nullptr;

// Event listener instancer.
static EventListenerInstancer* event_listener_instancer = nullptr;

// Default instancers are constructed and destroyed on Initialise and Shutdown, respectively.
struct DefaultInstancers {

	UniquePtr<ContextInstancer> context_default;
	UniquePtr<EventInstancer> event_default;

	// Basic elements
	ElementInstancerElement element_default;
	ElementInstancerTextDefault element_text_default;
	ElementInstancerGeneric<ElementImage> element_img;
	ElementInstancerGeneric<ElementHandle> element_handle;
	ElementInstancerGeneric<ElementDocument> element_body;

	// Control elements
	ElementInstancerGeneric<ElementForm> form;
	ElementInstancerGeneric<ElementFormControlInput> input;
	ElementInstancerGeneric<ElementFormControlDataSelect> dataselect;
	ElementInstancerGeneric<ElementFormControlSelect> select;

	ElementInstancerGeneric<ElementFormControlTextArea> textarea;
	ElementInstancerGeneric<ElementTextSelection> selection;
	ElementInstancerGeneric<ElementTabSet> tabset;

	ElementInstancerGeneric<ElementProgressBar> progressbar;

	ElementInstancerGeneric<ElementDataGrid> datagrid;
	ElementInstancerGeneric<ElementDataGridExpandButton> datagrid_expand;
	ElementInstancerGeneric<ElementDataGridCell> datagrid_cell;
	ElementInstancerGeneric<ElementDataGridRow> datagrid_row;

	// Decorators
	DecoratorTiledHorizontalInstancer decorator_tiled_horizontal;
	DecoratorTiledVerticalInstancer decorator_tiled_vertical;
	DecoratorTiledBoxInstancer decorator_tiled_box;
	DecoratorTiledImageInstancer decorator_image;
	DecoratorNinePatchInstancer decorator_ninepatch;
	DecoratorGradientInstancer decorator_gradient;

	// Font effects
	FontEffectBlurInstancer font_effect_blur;
	FontEffectGlowInstancer font_effect_glow;
	FontEffectOutlineInstancer font_effect_outline;
	FontEffectShadowInstancer font_effect_shadow;

	// Data binding views
	DataViewInstancerDefault<DataViewAttribute> data_view_attribute;
	DataViewInstancerDefault<DataViewClass> data_view_class;
	DataViewInstancerDefault<DataViewIf> data_view_if;
	DataViewInstancerDefault<DataViewVisible> data_view_visible;
	DataViewInstancerDefault<DataViewRml> data_view_rml;
	DataViewInstancerDefault<DataViewStyle> data_view_style;
	DataViewInstancerDefault<DataViewText> data_view_text;
	DataViewInstancerDefault<DataViewValue> data_view_value;

	DataViewInstancerDefault<DataViewFor> structural_data_view_for;

	// Data binding controllers
	DataControllerInstancerDefault<DataControllerValue> data_controller_value;
	DataControllerInstancerDefault<DataControllerEvent> data_controller_event;
};

static UniquePtr<DefaultInstancers> default_instancers;


Factory::Factory()
{
}

Factory::~Factory()
{
}


bool Factory::Initialise()
{
	default_instancers = MakeUnique<DefaultInstancers>();

	// Default context instancer
	if (!context_instancer)
	{
		default_instancers->context_default = MakeUnique<ContextInstancerDefault>();
		context_instancer = default_instancers->context_default.get();
	}

	// Default event instancer
	if (!event_instancer)
	{
		default_instancers->event_default = MakeUnique<EventInstancerDefault>();
		event_instancer = default_instancers->event_default.get();
	}

	// No default event listener instancer
	if (!event_listener_instancer)
		event_listener_instancer = nullptr;

	// Basic element instancers
	RegisterElementInstancer("*", &default_instancers->element_default);
	RegisterElementInstancer("img", &default_instancers->element_img);
	RegisterElementInstancer("#text", &default_instancers->element_text_default);
	RegisterElementInstancer("handle", &default_instancers->element_handle);
	RegisterElementInstancer("body", &default_instancers->element_body);

	// Control element instancers
	RegisterElementInstancer("form", &default_instancers->form);
	RegisterElementInstancer("input", &default_instancers->input);
	RegisterElementInstancer("dataselect", &default_instancers->dataselect);
	RegisterElementInstancer("select", &default_instancers->select);

	RegisterElementInstancer("textarea", &default_instancers->textarea);
	RegisterElementInstancer("#selection", &default_instancers->selection);
	RegisterElementInstancer("tabset", &default_instancers->tabset);

	RegisterElementInstancer("progressbar", &default_instancers->progressbar);

	RegisterElementInstancer("datagrid", &default_instancers->datagrid);
	RegisterElementInstancer("datagridexpand", &default_instancers->datagrid_expand);
	RegisterElementInstancer("#rmlctl_datagridcell", &default_instancers->datagrid_cell);
	RegisterElementInstancer("#rmlctl_datagridrow", &default_instancers->datagrid_row);

	// Decorator instancers
	RegisterDecoratorInstancer("tiled-horizontal", &default_instancers->decorator_tiled_horizontal);
	RegisterDecoratorInstancer("tiled-vertical", &default_instancers->decorator_tiled_vertical);
	RegisterDecoratorInstancer("tiled-box", &default_instancers->decorator_tiled_box);
	RegisterDecoratorInstancer("image", &default_instancers->decorator_image);
	RegisterDecoratorInstancer("ninepatch", &default_instancers->decorator_ninepatch);
	RegisterDecoratorInstancer("gradient", &default_instancers->decorator_gradient);

	// Font effect instancers
	RegisterFontEffectInstancer("blur", &default_instancers->font_effect_blur);
	RegisterFontEffectInstancer("glow", &default_instancers->font_effect_glow);
	RegisterFontEffectInstancer("outline", &default_instancers->font_effect_outline);
	RegisterFontEffectInstancer("shadow", &default_instancers->font_effect_shadow);

	// Data binding views
	RegisterDataViewInstancer(&default_instancers->data_view_attribute,      "attr",    false);
	RegisterDataViewInstancer(&default_instancers->data_view_class,          "class",   false);
	RegisterDataViewInstancer(&default_instancers->data_view_if,             "if",      false);
	RegisterDataViewInstancer(&default_instancers->data_view_visible,        "visible", false);
	RegisterDataViewInstancer(&default_instancers->data_view_rml,            "rml",     false);
	RegisterDataViewInstancer(&default_instancers->data_view_style,          "style",   false);
	RegisterDataViewInstancer(&default_instancers->data_view_text,           "text",    false);
	RegisterDataViewInstancer(&default_instancers->data_view_value,          "value",   false);
	RegisterDataViewInstancer(&default_instancers->structural_data_view_for, "for",     true );

	// Data binding controllers
	RegisterDataControllerInstancer(&default_instancers->data_controller_value, "value");
	RegisterDataControllerInstancer(&default_instancers->data_controller_event, "event");

	// XML node handlers
	XMLParser::RegisterNodeHandler("", MakeShared<XMLNodeHandlerDefault>());
	XMLParser::RegisterNodeHandler("body", MakeShared<XMLNodeHandlerBody>());
	XMLParser::RegisterNodeHandler("head", MakeShared<XMLNodeHandlerHead>());
	XMLParser::RegisterNodeHandler("template", MakeShared<XMLNodeHandlerTemplate>());

	// XML node handlers for control elements
	XMLParser::RegisterNodeHandler("datagrid", MakeShared<XMLNodeHandlerDataGrid>());
	XMLParser::RegisterNodeHandler("tabset", MakeShared<XMLNodeHandlerTabSet>());
	XMLParser::RegisterNodeHandler("textarea", MakeShared<XMLNodeHandlerTextArea>());

	return true;
}

void Factory::Shutdown()
{
	element_instancers.clear();

	decorator_instancers.clear();

	font_effect_instancers.clear();

	data_view_instancers.clear();
	structural_data_view_instancers.clear();
	structural_data_view_attribute_names.clear();

	context_instancer = nullptr;

	event_listener_instancer = nullptr;

	event_instancer = nullptr;

	XMLParser::ReleaseHandlers();

	default_instancers.reset();
}

// Registers the instancer to use when instancing contexts.
void Factory::RegisterContextInstancer(ContextInstancer* instancer)
{
	context_instancer = instancer;
}

// Instances a new context.
ContextPtr Factory::InstanceContext(const String& name)
{
	ContextPtr new_context = context_instancer->InstanceContext(name);
	if (new_context)
		new_context->SetInstancer(context_instancer);
	return new_context;
}

void Factory::RegisterElementInstancer(const String& name, ElementInstancer* instancer)
{
	element_instancers[StringUtilities::ToLower(name)] = instancer;
}

// Looks up the instancer for the given element
ElementInstancer* Factory::GetElementInstancer(const String& tag)
{
	ElementInstancerMap::iterator instancer_iterator = element_instancers.find(tag);
	if (instancer_iterator == element_instancers.end())
	{
		instancer_iterator = element_instancers.find("*");
		if (instancer_iterator == element_instancers.end())
			return nullptr;
	}

	return instancer_iterator->second;
}

// Instances a single element.
ElementPtr Factory::InstanceElement(Element* parent, const String& instancer_name, const String& tag, const XMLAttributes& attributes)
{
	ElementInstancer* instancer = GetElementInstancer(instancer_name);

	if (instancer)
	{
		ElementPtr element = instancer->InstanceElement(parent, tag, attributes);		

		// Process the generic attributes and bind any events
		if (element)
		{
			element->SetInstancer(instancer);
			element->SetAttributes(attributes);
			ElementUtilities::BindEventAttributes(element.get());

			PluginRegistry::NotifyElementCreate(element.get());
		}

		return element;
	}

	return nullptr;
}

// Instances a single text element containing a string.
bool Factory::InstanceElementText(Element* parent, const String& in_text)
{
	RMLUI_ASSERT(parent);

	String text;
	if (SystemInterface* system_interface = GetSystemInterface())
		system_interface->TranslateString(text, in_text);

	// If this text node only contains white-space we don't want to construct it.
	const bool only_white_space = std::all_of(text.begin(), text.end(), &StringUtilities::IsWhitespace);
	if (only_white_space)
		return true;

	// See if we need to parse it as RML, and whether the text contains data expressions (curly brackets).
	bool parse_as_rml = false;
	bool has_data_expression = false;

	bool inside_brackets = false;
	char previous = 0;
	for (const char c : text)
	{
		const char* error_str = XMLParseTools::ParseDataBrackets(inside_brackets, c, previous);
		if (error_str)
		{
			Log::Message(Log::LT_WARNING, "Failed to instance text element '%s'. %s", text.c_str(), error_str);
			return false;
		}

		if (inside_brackets)
			has_data_expression = true;
		else if (c == '<')
			parse_as_rml = true;

		previous = c;
	}

	// If the text contains RML elements then run it through the XML parser again.
	if (parse_as_rml)
	{
		RMLUI_ZoneScopedNC("InstanceStream", 0xDC143C);
		auto stream = MakeUnique<StreamMemory>(text.size() + 32);
		String tag = parent->GetContext()->GetDocumentsBaseTag();
		String open_tag = "<" + tag + ">";
		String close_tag = "</" + tag + ">";
		stream->Write(open_tag.c_str(), open_tag.size());
		stream->Write(text);
		stream->Write(close_tag.c_str(), close_tag.size());
		stream->Seek(0, SEEK_SET);

		InstanceElementStream(parent, stream.get());
	}
	else
	{
		RMLUI_ZoneScopedNC("InstanceText", 0x8FBC8F);
		
		// Attempt to instance the element.
		XMLAttributes attributes;

		// If we have curly brackets in the text, we tag the element so that the appropriate data view (DataViewText) is constructed.
		if (has_data_expression)
			attributes.emplace("data-text", Variant());

		ElementPtr element = Factory::InstanceElement(parent, "#text", "#text", attributes);
		if (!element)
		{
			Log::Message(Log::LT_ERROR, "Failed to instance text element '%s', instancer returned nullptr.", text.c_str());
			return false;
		}

		// Assign the element its text value.
		ElementText* text_element = rmlui_dynamic_cast< ElementText* >(element.get());
		if (!text_element)
		{
			Log::Message(Log::LT_ERROR, "Failed to instance text element '%s'. Found type '%s', was expecting a derivative of ElementText.", text.c_str(), rmlui_type_name(*element));
			return false;
		}

		text_element->SetText(text);

		// Add to active node.
		parent->AppendChild(std::move(element));
	}

	return true;
}

// Instances a element tree based on the stream
bool Factory::InstanceElementStream(Element* parent, Stream* stream)
{
	XMLParser parser(parent);
	parser.Parse(stream);
	return true;
}

// Instances a element tree based on the stream
ElementPtr Factory::InstanceDocumentStream(Context* context, Stream* stream)
{
	RMLUI_ZoneScoped;
	RMLUI_ASSERT(context);

	ElementPtr element = Factory::InstanceElement(nullptr, context->GetDocumentsBaseTag(), context->GetDocumentsBaseTag(), XMLAttributes());
	if (!element)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document, instancer returned nullptr.");
		return nullptr;
	}

	ElementDocument* document = rmlui_dynamic_cast< ElementDocument* >(element.get());
	if (!document)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document element. Found type '%s', was expecting derivative of ElementDocument.", rmlui_type_name(*element));
		return nullptr;
	}

	document->context = context;

	XMLParser parser(element.get());
	parser.Parse(stream);

	return element;
}


// Registers an instancer that will be used to instance decorators.
void Factory::RegisterDecoratorInstancer(const String& name, DecoratorInstancer* instancer)
{
	RMLUI_ASSERT(instancer);
	decorator_instancers[StringUtilities::ToLower(name)] = instancer;
}

// Retrieves a decorator instancer registered with the factory.
DecoratorInstancer* Factory::GetDecoratorInstancer(const String& name)
{
	auto iterator = decorator_instancers.find(name);
	if (iterator == decorator_instancers.end())
		return nullptr;
	
	return iterator->second;
}

// Registers an instancer that will be used to instance font effects.
void Factory::RegisterFontEffectInstancer(const String& name, FontEffectInstancer* instancer)
{
	RMLUI_ASSERT(instancer);
	font_effect_instancers[StringUtilities::ToLower(name)] = instancer;
}

FontEffectInstancer* Factory::GetFontEffectInstancer(const String& name)
{
	auto iterator = font_effect_instancers.find(name);
	if (iterator == font_effect_instancers.end())
		return nullptr;

	return iterator->second;
}


// Creates a style sheet containing the passed in styles.
SharedPtr<StyleSheet> Factory::InstanceStyleSheetString(const String& string)
{
	auto memory_stream = MakeUnique<StreamMemory>((const byte*) string.c_str(), string.size());
	return InstanceStyleSheetStream(memory_stream.get());
}

// Creates a style sheet from a file.
SharedPtr<StyleSheet> Factory::InstanceStyleSheetFile(const String& file_name)
{
	auto file_stream = MakeUnique<StreamFile>();
	file_stream->Open(file_name);
	return InstanceStyleSheetStream(file_stream.get());
}

// Creates a style sheet from an Stream.
SharedPtr<StyleSheet> Factory::InstanceStyleSheetStream(Stream* stream)
{
	SharedPtr<StyleSheet> style_sheet = MakeShared<StyleSheet>();
	if (style_sheet->LoadStyleSheet(stream))
	{
		return style_sheet;
	}
	return nullptr;
}

// Clears the style sheet cache. This will force style sheets to be reloaded.
void Factory::ClearStyleSheetCache()
{
	StyleSheetFactory::ClearStyleSheetCache();
}

/// Clears the template cache. This will force templates to be reloaded.
void Factory::ClearTemplateCache()
{
	TemplateCache::Clear();
}

// Registers an instancer for all RmlEvents
void Factory::RegisterEventInstancer(EventInstancer* instancer)
{
	event_instancer = instancer;
}

// Instance an event object.
EventPtr Factory::InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible)
{
	EventPtr event = event_instancer->InstanceEvent(target, id, type, parameters, interruptible);
	if (event)
		event->instancer = event_instancer;
	return event;
}

// Register an instancer for all event listeners
void Factory::RegisterEventListenerInstancer(EventListenerInstancer* instancer)
{
	event_listener_instancer = instancer;
}

// Instance an event listener with the given string
EventListener* Factory::InstanceEventListener(const String& value, Element* element)
{
	// If we have an event listener instancer, use it
	if (event_listener_instancer)
		return event_listener_instancer->InstanceEventListener(value, element);

	return nullptr;
}

void Factory::RegisterDataViewInstancer(DataViewInstancer* instancer, const String& name, bool is_structural_view)
{
	bool inserted = false;
	if (is_structural_view)
	{
		inserted = structural_data_view_instancers.emplace(name, instancer).second;
		if (inserted)
			structural_data_view_attribute_names.push_back(String("data-") + name);
	}
	else
	{
		inserted = data_view_instancers.emplace(name, instancer).second;
	}
	
	if (!inserted)
		Log::Message(Log::LT_WARNING, "Could not register data view instancer '%s'. The given name is already registered.", name.c_str());
}

void Factory::RegisterDataControllerInstancer(DataControllerInstancer* instancer, const String& name)
{
	bool inserted = data_controller_instancers.emplace(name, instancer).second;
	if (!inserted)
		Log::Message(Log::LT_WARNING, "Could not register data controller instancer '%s'. The given name is already registered.", name.c_str());
}

DataViewPtr Factory::InstanceDataView(const String& type_name, Element* element, bool is_structural_view)
{
	RMLUI_ASSERT(element);

	if (is_structural_view)
	{
		auto it = structural_data_view_instancers.find(type_name);
		if (it != structural_data_view_instancers.end())
			return it->second->InstanceView(element);
	}
	else
	{
		auto it = data_view_instancers.find(type_name);
		if (it != data_view_instancers.end())
			return it->second->InstanceView(element);
	}
	return nullptr;
}

DataControllerPtr Factory::InstanceDataController(const String& type_name, Element* element)
{
	auto it = data_controller_instancers.find(type_name);
	if (it != data_controller_instancers.end())
		return it->second->InstanceController(element);
	return DataControllerPtr();
}

const StringList& Factory::GetStructuralDataViewAttributeNames()
{
	return structural_data_view_attribute_names;
}

} // namespace Rml
