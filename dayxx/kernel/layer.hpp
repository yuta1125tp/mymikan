/**
 * @file layer.hpp
 * @brief 重ね合わせ処理を提供する
 * 
 */

#pragma once
#include <memory>
#include <map>
#include <vector>

#include "graphics.hpp"
#include "window.hpp"

/**
 * @brief Layerは1つの層を表す。
 * 
 * 現状は1つのウインドウしか保持できない設計だけど、将来的には複数のウインドウを持ち得る。
 */
class Layer
{
public:
    Layer(unsigned int id = 0);
    unsigned int ID() const;
    Layer &SetWindow(const std::shared_ptr<Window> &window);
    std::shared_ptr<Window> GetWindow() const;

    Layer &Move(Vector2D<int> pos);
    Layer &MoveRelative(Vector2D<int> pos_diff);

    /**
     * @brief writerに現在設定されているウインドウの内容を描画する
     * 
     * @param writer 
     */
    void DrawTo(PixelWriter &writer) const;

private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
};

/**
 * @brief LayerManagerは複数のレイヤを管理する。
 * 
 */
class LayerManager
{
public:
    /**
     * @brief Drawメソッドなどで描画する際の描画先を設定する
     * 
     * @param writer 
     */
    void SetWriter(PixelWriter *writer);

    /**
     * @brief 新しいレイヤを生成して参照を返す。
     * 新しく生成されたレイヤの実体はLayerManager内部のコンテナで保持される。
     * 
     * @return Layer& 新しく生成したレイヤの参照
     */
    Layer &NewLayer();

    /**
     * @brief 現在表示状態にあるレイヤを描画する
     * 
     */
    void Draw() const;

    /**
     * @brief レイヤの位置情報を指定された絶対座標へと更新する。再描画はしない
     * 
     * @param id 
     * @param new_position 
     */
    void Move(unsigned int id, Vector2D<int> new_position);
    /**
     * @brief レイヤの位置情報を指定された相対座標へと更新する。再描画はしない
     * 
     * @param id 
     * @param new_position 
     */
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

    /**
     * @brief レイヤの高さ方向の位置を指定された位置に移動する。
     * 
     * new_heightに負を指定するとレイヤは非表示にする。0以上ならその高さにする。
     * 現在のレイヤ数以上の数字を指定した場合は最前面のレイヤにする。
     * @param id 
     * @param new_height 
     */
    void UpDown(unsigned int id, int new_height);
    /**
     * @brief レイヤを非表示にする
     * 
     * @param id 
     */
    void Hide(unsigned int id);

private:
    PixelWriter *writer_{nullptr};
    std::vector<std::unique_ptr<Layer>> layers_{};
    std::vector<Layer *> layer_stack_{};
    unsigned int latest_id_{0};

    /**
     * @brief 指定したidのLayerを探す。見つからなかったらnullptrを返す。
     * 
     * @param id 
     * @return Layer* 
     */
    Layer *FindLayer(unsigned int id);
};

extern LayerManager *layer_manager;
