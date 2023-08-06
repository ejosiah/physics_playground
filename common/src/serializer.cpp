#include "serializer.h"

#include "snap.h"
#include <yaml-cpp/yaml.h>
#include <optional>

namespace yaml {


    template<typename T>
    void capture(std::any any, YAML::Emitter& emitter) {
        if(any.type() == typeid(Snap::Sequence<T>)){
            emitter << YAML::Value;
            const auto& seq = std::any_cast<Snap::Sequence<T>>(any);
            emitter << YAML::BeginSeq;
            for(const auto& value : seq) {
                emitter << std::any_cast<T>(value);
            }
            emitter << YAML::EndSeq;
        }
        if(any.type() == typeid(T)){
            emitter << YAML::Value << std::any_cast<T>(any);
        }
    }

    YAML::Emitter& process(const Snap::Snapshot& snapshot, YAML::Emitter& emitter) {
        emitter << YAML::BeginMap;
        for(auto& [key, value] : snapshot) {
            emitter << YAML::Key <<  key;
            if(value.type() == typeid(Snap::Snapshot)){
                process(std::any_cast<Snap::Snapshot>(value), emitter);
            }else {
                capture<bool>(value, emitter);
                capture<int>(value, emitter);
                capture<unsigned>(value, emitter);
                capture<long>(value, emitter);
                capture<unsigned long>(value, emitter);
                capture<float>(value, emitter);
                capture<double>(value, emitter);
                capture<std::string>(value, emitter);

                capture<Snap::Sequence<int>>(value, emitter);
                capture<Snap::Sequence<unsigned>>(value, emitter);
                capture<Snap::Sequence<long>>(value, emitter);
                capture<Snap::Sequence<unsigned long>>(value, emitter);
                capture<Snap::Sequence<float>>(value, emitter);
                capture<Snap::Sequence<double>>(value, emitter);
                capture<Snap::Sequence<std::string>>(value, emitter);
            }
        }
        emitter << YAML::EndMap;

        return emitter;
    }

//    auto serialize(const auto& object) -> std::string {
//        YAML::Emitter emitter;
//        return process(object.snapshot(), emitter).c_str();
//    }

    auto serialize(const Snap::Snapshot& snapshot) -> std::string {
        YAML::Emitter emitter;
        return process(snapshot, emitter).c_str();
    }
}