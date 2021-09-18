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
    /**　@brief 指定したIDを持つレイヤインスタンスを生成する */
    Layer(unsigned int id = 0);
    /**　@brief このインスタンスのIDを返す
     * IDは1以上の整数（みかん本263p）
     */
    unsigned int ID() const;

    /**
     * @brief ウインドウを設定する。既存のウインドウはこのレイヤーから外れる。
     * 
     * @param window 
     * @return Layer& 
     */
    Layer &SetWindow(const std::shared_ptr<Window> &window);
    /**　@brief 設定されたウインドウを返す */
    std::shared_ptr<Window> GetWindow() const;
    /**　@brief レイヤの原点座標を取得する */
    Vector2D<int> GetPosition() const;
    /** @brief レイヤのドラッグ可否のフラグを操作、tureならドラッグ可能レイヤ、コンストラクタではfalseで初期化済み*/
    Layer &SetDraggable(bool draggable);
    /** @brief レイヤのドラッグ可否のフラグを返す*/
    bool IsDraggable() const;

    /**　@brief レイヤの位置情報を指定した絶対座標へ更新する。再描画はしない */
    Layer &Move(Vector2D<int> pos);
    /**　@brief レイヤの位置情報を指定した相対座標へ更新する。再描画はしない */
    Layer &MoveRelative(Vector2D<int> pos_diff);

    /**
     * @brief writerに現在設定されているウインドウの内容のうち指定されたエリアの情報をscreenに描画する
     * 
     * @param writer 
     */
    void DrawTo(FrameBuffer &screen, const Rectangle<int> &area) const;

private:
    unsigned int id_;
    Vector2D<int> pos_{};
    std::shared_ptr<Window> window_{};
    bool draggable_{false};
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
    void SetWriter(FrameBuffer *screen);

    /**
     * @brief 新しいレイヤを生成して参照を返す。
     * 新しく生成されたレイヤの実体はLayerManager内部のコンテナで保持される。
     * 
     * @return Layer& 新しく生成したレイヤの参照
     */
    Layer &NewLayer();

    /**
     * @brief 現在表示状態にあるレイヤをエリアを絞り描画する
     * 
     */
    void Draw(const Rectangle<int> &area) const;

    /**
     * @brief 指定したIDのレイヤに設定されているウインドウの描画領域内を描画する
     * 
     */
    void Draw(unsigned int id) const;

    /**
     * @brief レイヤの位置情報を指定された絶対座標へと更新する。再描画する
     * 
     * @param id 
     * @param new_pos 
     */
    void Move(unsigned int id, Vector2D<int> new_pos);
    /**
     * @brief レイヤの位置情報を指定された相対座標へと更新する。再描画する
     * 
     * @param id 
     * @param pos_diff 
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

    /** @brief 指定された座標にウィンドウを持つ最も上に表示されているレイヤーを探す。 */
    Layer *FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const;

private:
    FrameBuffer *screen_{nullptr};
    /** @brief バックバッファ[みかん本10.6章] */
    mutable FrameBuffer back_buffer_{};
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

/**
 * @brief Layer（Back Ground LayerとConsole Layer）とLayerManagerに関する初期化処理
 * KernelMainNewStackから呼び出す。
 * 
 */
void InitializeLayer();
