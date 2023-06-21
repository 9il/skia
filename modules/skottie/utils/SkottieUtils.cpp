/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/skottie/src/SkottieJson.h"

#include "modules/skottie/utils/SkottieUtils.h"

#include "include/core/SkImage.h"

namespace skottie_utils {

class CustomPropertyManager::PropertyInterceptor final : public skottie::PropertyObserver {
public:
    explicit PropertyInterceptor(CustomPropertyManager* mgr) : fMgr(mgr) {}

    void onColorProperty(const char node_name[],
                         const LazyHandle<skottie::ColorPropertyHandle>& c) override {
        const auto key = fMgr->acceptKey(node_name, ".Color");
        if (!key.empty()) {
            fMgr->fColorMap[key].push_back(c());
        }
    }

    void onOpacityProperty(const char node_name[],
                           const LazyHandle<skottie::OpacityPropertyHandle>& o) override {
        const auto key = fMgr->acceptKey(node_name, ".Opacity");
        if (!key.empty()) {
            fMgr->fOpacityMap[key].push_back(o());
        }
    }

    void onTransformProperty(const char node_name[],
                             const LazyHandle<skottie::TransformPropertyHandle>& t) override {
        const auto key = fMgr->acceptKey(node_name, ".Transform");
        if (!key.empty()) {
            fMgr->fTransformMap[key].push_back(t());
        }
    }

    void onTextProperty(const char node_name[],
                        const LazyHandle<skottie::TextPropertyHandle>& t) override {
        const auto key = fMgr->acceptKey(node_name, ".Text");
        if (!key.empty()) {
            fMgr->fTextMap[key].push_back(t());
        }
    }

    void onEnterNode(const char node_name[], PropertyObserver::NodeType node_type) override {
        if (node_name == nullptr) {
            return;
        }
        fMgr->fCurrentNode =
                fMgr->fCurrentNode.empty() ? node_name : fMgr->fCurrentNode + "." + node_name;
    }

    void onLeavingNode(const char node_name[], PropertyObserver::NodeType node_type) override {
        if (node_name == nullptr) {
            return;
        }
        auto length = strlen(node_name);
        fMgr->fCurrentNode =
                fMgr->fCurrentNode.length() > length
                        ? fMgr->fCurrentNode.substr(
                                  0, fMgr->fCurrentNode.length() - strlen(node_name) - 1)
                        : "";
    }

private:
    CustomPropertyManager* fMgr;
};

class CustomPropertyManager::MarkerInterceptor final : public skottie::MarkerObserver {
public:
    explicit MarkerInterceptor(CustomPropertyManager* mgr) : fMgr(mgr) {}

    void onMarker(const char name[], float t0, float t1) override {
        // collect all markers
        fMgr->fMarkers.push_back({ std::string(name), t0, t1 });
    }

private:
    CustomPropertyManager* fMgr;
};

CustomPropertyManager::CustomPropertyManager(Mode mode, const char* prefix)
    : fMode(mode)
    , fPrefix(prefix ? prefix : "$")
    , fPropertyInterceptor(sk_make_sp<PropertyInterceptor>(this))
    , fMarkerInterceptor(sk_make_sp<MarkerInterceptor>(this)) {}

CustomPropertyManager::~CustomPropertyManager() = default;

std::string CustomPropertyManager::acceptKey(const char* name, const char* suffix) const {
    if (!SkStrStartsWith(name, fPrefix.c_str())) {
        return std::string();
    }

    return fMode == Mode::kCollapseProperties
            ? std::string(name)
            : fCurrentNode + suffix;
}

sk_sp<skottie::PropertyObserver> CustomPropertyManager::getPropertyObserver() const {
    return fPropertyInterceptor;
}

sk_sp<skottie::MarkerObserver> CustomPropertyManager::getMarkerObserver() const {
    return fMarkerInterceptor;
}

template <typename T>
std::vector<CustomPropertyManager::PropKey>
CustomPropertyManager::getProps(const PropMap<T>& container) const {
    std::vector<PropKey> props;

    for (const auto& prop_list : container) {
        SkASSERT(!prop_list.second.empty());
        props.push_back(prop_list.first);
    }

    return props;
}

template <typename V, typename T>
V CustomPropertyManager::get(const PropKey& key, const PropMap<T>& container) const {
    auto prop_group = container.find(key);

    return prop_group == container.end()
            ? V()
            : prop_group->second.front()->get();
}

template <typename V, typename T>
bool CustomPropertyManager::set(const PropKey& key, const V& val, const PropMap<T>& container) {
    auto prop_group = container.find(key);

    if (prop_group == container.end()) {
        return false;
    }

    for (auto& handle : prop_group->second) {
        handle->set(val);
    }

    return true;
}

std::vector<CustomPropertyManager::PropKey>
CustomPropertyManager::getColorProps() const {
    return this->getProps(fColorMap);
}

skottie::ColorPropertyValue CustomPropertyManager::getColor(const PropKey& key) const {
    return this->get<skottie::ColorPropertyValue>(key, fColorMap);
}

bool CustomPropertyManager::setColor(const PropKey& key, const skottie::ColorPropertyValue& c) {
    return this->set(key, c, fColorMap);
}

std::vector<CustomPropertyManager::PropKey>
CustomPropertyManager::getOpacityProps() const {
    return this->getProps(fOpacityMap);
}

skottie::OpacityPropertyValue CustomPropertyManager::getOpacity(const PropKey& key) const {
    return this->get<skottie::OpacityPropertyValue>(key, fOpacityMap);
}

bool CustomPropertyManager::setOpacity(const PropKey& key, const skottie::OpacityPropertyValue& o) {
    return this->set(key, o, fOpacityMap);
}

std::vector<CustomPropertyManager::PropKey>
CustomPropertyManager::getTransformProps() const {
    return this->getProps(fTransformMap);
}

skottie::TransformPropertyValue CustomPropertyManager::getTransform(const PropKey& key) const {
    return this->get<skottie::TransformPropertyValue>(key, fTransformMap);
}

bool CustomPropertyManager::setTransform(const PropKey& key,
                                         const skottie::TransformPropertyValue& t) {
    return this->set(key, t, fTransformMap);
}

std::vector<CustomPropertyManager::PropKey>
CustomPropertyManager::getTextProps() const {
    return this->getProps(fTextMap);
}

skottie::TextPropertyValue CustomPropertyManager::getText(const PropKey& key) const {
    return this->get<skottie::TextPropertyValue>(key, fTextMap);
}

bool CustomPropertyManager::setText(const PropKey& key, const skottie::TextPropertyValue& o) {
    return this->set(key, o, fTextMap);
}

namespace {

class ExternalAnimationLayer final : public skottie::ExternalLayer {
public:
    ExternalAnimationLayer(sk_sp<skottie::Animation> anim, const SkSize& size)
        : fAnimation(std::move(anim))
        , fSize(size) {}

private:
    void render(SkCanvas* canvas, double t) override {
        fAnimation->seekFrameTime(t);

        // The main animation will layer-isolate if needed - we don't want the nested animation
        // to override that decision.
        const auto flags = skottie::Animation::RenderFlag::kSkipTopLevelIsolation;
        const auto dst_rect = SkRect::MakeSize(fSize);
        fAnimation->render(canvas, &dst_rect, flags);
    }

    const sk_sp<skottie::Animation> fAnimation;
    const SkSize                    fSize;
};

} // namespace

ExternalAnimationPrecompInterceptor::ExternalAnimationPrecompInterceptor(
        sk_sp<skresources::ResourceProvider> rprovider,
        const char prefixp[])
    : fResourceProvider(std::move(rprovider))
    , fPrefix(prefixp) {}

ExternalAnimationPrecompInterceptor::~ExternalAnimationPrecompInterceptor() = default;

sk_sp<skottie::ExternalLayer> ExternalAnimationPrecompInterceptor::onLoadPrecomp(
        const char[], const char name[], const SkSize& size) {
    if (0 != strncmp(name, fPrefix.c_str(), fPrefix.size())) {
        return nullptr;
    }

    auto data = fResourceProvider->load("", name + fPrefix.size());
    if (!data) {
        return nullptr;
    }

    auto anim = skottie::Animation::Builder()
                    .setPrecompInterceptor(sk_ref_sp(this))
                    .setResourceProvider(fResourceProvider)
                    .make(static_cast<const char*>(data->data()), data->size());

    return anim ? sk_make_sp<ExternalAnimationLayer>(std::move(anim), size)
                : nullptr;
}

class ImageAssetProxy final : public skresources::ImageAsset {
public:
    ImageAssetProxy() {}

    // always returns true in case Image asset is swapped during playback
    bool isMultiFrame() override { return true; }

    FrameData getFrameData(float t) override {
        if (fImageAsset) {
            return fImageAsset->getFrameData(t);
        }
        return {nullptr , SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNearest),
            SkMatrix::I(), SizeFit::kCenter};
    }

    void setImageAsset (sk_sp<skresources::ImageAsset> asset) {
        fImageAsset = std::move(asset);
    }
private:
    sk_sp<skresources::ImageAsset> fImageAsset;
};

/**
 * An implementation of ResourceProvider designed for Lottie template asset substitution (images,
 * audio, etc)
 */
class SlotManager::SlottableResourceProvider final : public skresources::ResourceProviderProxyBase {
public:
    SlottableResourceProvider(std::vector<SlotInfo> slotInfos,
                              sk_sp<skresources::ResourceProvider> proxy)
        : skresources::ResourceProviderProxyBase(std::move(proxy)) {
        for (const auto &s : slotInfos) {
            if (s.type == SlotType::kImage) {
                fImageAssetMap[s.slotID] = sk_make_sp<ImageAssetProxy>();
            }
        }
    }

    // This implementation depends on slot ID being passed through id instead of asset ID when slots
    // are present
    sk_sp<skresources::ImageAsset> loadImageAsset(const char resource_path[],
                                                  const char name[],
                                                  const char slot_name[]) const override {
        const auto it = fImageAssetMap.find(slot_name);
        auto imageAssetProxy = it == fImageAssetMap.end() ? nullptr : it->second;
        if (fProxy) {
            imageAssetProxy->setImageAsset(fProxy->loadImageAsset(resource_path, name, slot_name));
        }
        return std::move(imageAssetProxy);
    }

private:
    std::unordered_map<std::string, sk_sp<ImageAssetProxy>> fImageAssetMap;
    friend class SlotManager;
};

/**
 * An implementation of PropertyObserver designed for Lottie template property substitution (color,
 * text, etc)
 *
 * PropertyObserver looks for slottable nodes then manipulates their PropertyValue on the fly
 *
 */
class SlotManager::SlottablePropertyObserver final : public skottie::PropertyObserver {
public:
    SlottablePropertyObserver(std::vector<SlotInfo> slotInfos,
                              sk_sp<skottie::PropertyObserver> proxy)
        : fProxy(proxy) {
        for (const auto &s : slotInfos) {
            switch (s.type) {
            case SlotType::kColor:
                fColorMap[s.slotID] = std::vector<std::unique_ptr<skottie::ColorPropertyHandle>>();
                break;
            case SlotType::kOpacity:
                fOpacityMap[s.slotID] =
                    std::vector<std::unique_ptr<skottie::OpacityPropertyHandle>>();
                break;
            case SlotType::kText:
                fTextMap[s.slotID] = std::vector<std::unique_ptr<skottie::TextPropertyHandle>>();
                break;
            default:
                SkDebugf("Unsupported slot type: %s: %d\n", s.slotID.c_str(), s.type);
                break;
            }
        }
    }

    void onColorProperty(const char node_name[],
                         const LazyHandle<skottie::ColorPropertyHandle>& c) override {
        if (node_name) {
            const auto it = fColorMap.find(node_name);
            if (it != fColorMap.end()) {
                fColorMap[node_name].push_back(c());
            }
        }
        if (fProxy) {
            fProxy->onColorProperty(node_name, c);
        }
    }

    void onOpacityProperty(const char node_name[],
                           const LazyHandle<skottie::OpacityPropertyHandle>& o) override {
        if (node_name) {
            const auto it = fOpacityMap.find(node_name);
            if (it != fOpacityMap.end()) {
                fOpacityMap[node_name].push_back(o());
            }
        }
        if (fProxy) {
            fProxy->onOpacityProperty(node_name, o);
        }
    }

    void onTextProperty(const char node_name[],
                        const LazyHandle<skottie::TextPropertyHandle>& t) override {
        const auto it = fTextMap.find(node_name);
        if (it != fTextMap.end()) {
            fTextMap[node_name].push_back(t());
        }
        if (fProxy) {
            fProxy->onTextProperty(node_name, t);
        }
    }

    void onTransformProperty(const char node_name[],
                             const LazyHandle<skottie::TransformPropertyHandle>& t) override {
        if (fProxy) {
            fProxy->onTransformProperty(node_name, t);
        }
    }

    void onEnterNode(const char node_name[], NodeType node_type) override {
        if (fProxy) {
            fProxy->onEnterNode(node_name, node_type);
        }
    }

    void onLeavingNode(const char node_name[], NodeType node_type) override {
        if (fProxy) {
            fProxy->onLeavingNode(node_name, node_type);
        }
    }
private:
    using SlotID = std::string;

    std::unordered_map<SlotID, std::vector<std::unique_ptr<skottie::ColorPropertyHandle>>>
        fColorMap;
    std::unordered_map<SlotID, std::vector<std::unique_ptr<skottie::OpacityPropertyHandle>>>
        fOpacityMap;
    std::unordered_map<SlotID, std::vector<std::unique_ptr<skottie::TextPropertyHandle>>>
        fTextMap;

    sk_sp<skottie::PropertyObserver> fProxy;

    friend class SlotManager;
};

SlotManager::SlotManager(const SkString path, sk_sp<skresources::ResourceProvider> rpProxy,
                         sk_sp<skottie::PropertyObserver> poProxy) {
    parseSlotIDsFromFileName(path);
    fResourceProvider = sk_make_sp<SlottableResourceProvider>(fSlotInfos, rpProxy);
    fPropertyObserver = sk_make_sp<SlottablePropertyObserver>(fSlotInfos, poProxy);
}

// TODO: replace with parse from SkData (grab SkData from filename instead)
void SlotManager::parseSlotIDsFromFileName(SkString path) {
    if (const auto data = SkData::MakeFromFileName(path.c_str())) {
        const skjson::DOM dom(static_cast<const char*>(data->data()), data->size());
        if (dom.root().is<skjson::ObjectValue>()) {
            const auto& json = dom.root().as<skjson::ObjectValue>();
            if (const skjson::ObjectValue* jslots = json["slots"]) {
                for (const auto& member : *jslots) {
                    auto slotID = member.fKey.begin();
                    const skjson::ObjectValue* jslot = member.fValue;
                    int type = skottie::ParseDefault<int>((*jslot)["t"], -1);
                    fSlotInfos.push_back({slotID, type});
                }
            }
        }
    }
}

void SlotManager::setColorSlot(std::string slotID, SkColor color) {
    const auto it = fPropertyObserver->fColorMap.find(slotID);
    if (it != fPropertyObserver->fColorMap.end()) {
        for (auto& handle : fPropertyObserver->fColorMap[slotID]) {
            handle->set(color);
        }
    }
}

void SlotManager::setOpacitySlot(std::string slotID, SkScalar opacity) {
    const auto it = fPropertyObserver->fOpacityMap.find(slotID);
    if (it != fPropertyObserver->fOpacityMap.end()) {
        for (auto& handle : fPropertyObserver->fOpacityMap[slotID]) {
            handle->set(opacity);
        }
    }
}

void SlotManager::setTextStringSlot(std::string slotID, SkString text) {
    const auto it = fPropertyObserver->fTextMap.find(slotID);
    if (it != fPropertyObserver->fTextMap.end()) {
        for (auto& handle : fPropertyObserver->fTextMap[slotID]) {
            auto tVal = handle->get();
            tVal.fText = text;
            handle->set(tVal);
        }
    }
}

void SlotManager::setImageSlot(std::string slotID, sk_sp<skresources::ImageAsset> img) {
    const auto it = fResourceProvider->fImageAssetMap.find(slotID);
    if (it != fResourceProvider->fImageAssetMap.end()) {
        fResourceProvider->fImageAssetMap[slotID]->setImageAsset(std::move(img));
    }
}

// forwards onLoad to proxy resource provider
void SlotManager::setImageSlot(std::string slotID, const char path[], const char name[],
                               const char id[]) {
    const auto it = fResourceProvider->fImageAssetMap.find(slotID);
    if (it != fResourceProvider->fImageAssetMap.end()) {
        fResourceProvider->fImageAssetMap[slotID]->setImageAsset(
            fResourceProvider->fProxy
            ? fResourceProvider->fProxy->loadImageAsset(path, name, id)
            : nullptr);
    }
}

sk_sp<skresources::ResourceProvider> SlotManager::getResourceProvider() const {
    return fResourceProvider;
}

sk_sp<skottie::PropertyObserver> SlotManager::getPropertyObserver() const {
    return fPropertyObserver;
}

} // namespace skottie_utils