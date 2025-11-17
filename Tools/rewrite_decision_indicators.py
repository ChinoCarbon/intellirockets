import json
import os


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def collect_indicators(data):
    alg = []
    subsys = []
    for cat in data.get("categories", []):
        name = cat.get("name", "")
        cid = cat.get("id", "")
        if name == "决策算法指标" or cid == "3.4":
            for sub in cat.get("subCategories", []):
                alg.extend(sub.get("indicators", []))
        elif name == "决策分系统指标" or cid == "3.5":
            for sub in cat.get("subCategories", []):
                subsys.extend(sub.get("indicators", []))
    return alg, subsys


def group_by_prefix(indicators):
    groups = {}
    order = []
    for ind in indicators:
        name = ind.get("name", "")
        idx = name.find("性-")
        if idx != -1:
            prefix = name[: idx + 1]  # 包含“性”
        else:
            prefix = "其他"
        if prefix not in groups:
            groups[prefix] = []
            order.append(prefix)
        groups[prefix].append(ind)
    return order, groups


def build_category(title, order, groups):
    return {
        "name": title,
        "subCategories": [
            {"name": prefix, "indicators": groups[prefix]} for prefix in order
        ],
    }


def main():
    root = os.path.dirname(os.path.dirname(__file__))
    path = os.path.join(root, "Content", "Config", "DecisionIndicators.json")
    data = load_json(path)

    alg_inds, subsys_inds = collect_indicators(data)

    alg_order, alg_groups = group_by_prefix(alg_inds)
    subsys_order, subsys_groups = group_by_prefix(subsys_inds)

    new_data = {
        "categories": [
            build_category("决策算法指标", alg_order, alg_groups),
            build_category("决策分系统指标", subsys_order, subsys_groups),
        ]
    }

    save_json(path, new_data)
    print("Rewrote DecisionIndicators.json with grouped categories.")


if __name__ == "__main__":
    main()


