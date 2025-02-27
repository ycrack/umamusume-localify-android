package com.kimjio.umamusumelocalify.settings.preference;

import android.content.Context;
import android.content.res.TypedArray;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import com.kimjio.umamusumelocalify.settings.R;

import java.text.DecimalFormat;

public class FloatSeekBarPreference extends Preference {
    private static final String TAG = "FloatSeekBarPreference";

    @SuppressWarnings("WeakerAccess")
    float mSeekBarValue;
    @SuppressWarnings("WeakerAccess")
    float mMin;
    private float mMax;
    private float mSeekBarIncrement;
    @SuppressWarnings("WeakerAccess")
    boolean mTrackingTouch;
    @SuppressWarnings("WeakerAccess")
    SeekBar mSeekBar;
    private TextView mSeekBarValueTextView;
    // Whether the SeekBar should respond to the left/right keys
    @SuppressWarnings("WeakerAccess")
    boolean mAdjustable;
    // Whether to show the SeekBar value TextView next to the bar
    private boolean mShowSeekBarValue;
    // Whether the SeekBarPreference should continuously save the Seekbar value while it is being
    // dragged.
    @SuppressWarnings("WeakerAccess")
    boolean mUpdatesContinuously;
    /**
     * Listener reacting to the {@link SeekBar} changing value by the user
     */
    private final SeekBar.OnSeekBarChangeListener mSeekBarChangeListener = new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            if (fromUser && (mUpdatesContinuously || !mTrackingTouch)) {
                syncValueInternal(seekBar);
            } else {
                // We always want to update the text while the seekbar is being dragged
                updateLabelValue(Math.max(progress * mSeekBarIncrement + mMin, mMin));
            }
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            mTrackingTouch = true;
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            mTrackingTouch = false;
            if (seekBar.getProgress() * mSeekBarIncrement + mMin != mSeekBarValue) {
                syncValueInternal(seekBar);
            }
        }
    };

    /**
     * Listener reacting to the user pressing DPAD left/right keys if {@code
     * adjustable} attribute is set to true; it transfers the key presses to the {@link SeekBar}
     * to be handled accordingly.
     */
    private final View.OnKeyListener mSeekBarKeyListener = new View.OnKeyListener() {
        @Override
        public boolean onKey(View v, int keyCode, KeyEvent event) {
            if (event.getAction() != KeyEvent.ACTION_DOWN) {
                return false;
            }

            if (!mAdjustable && (keyCode == KeyEvent.KEYCODE_DPAD_LEFT
                    || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT)) {
                // Right or left keys are pressed when in non-adjustable mode; Skip the keys.
                return false;
            }

            // We don't want to propagate the click keys down to the SeekBar view since it will
            // create the ripple effect for the thumb.
            if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER || keyCode == KeyEvent.KEYCODE_ENTER) {
                return false;
            }

            if (mSeekBar == null) {
                Log.e(TAG, "SeekBar view is null and hence cannot be adjusted.");
                return false;
            }
            return mSeekBar.onKeyDown(keyCode, event);
        }
    };

    public FloatSeekBarPreference(
            @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);

        TypedArray a = context.obtainStyledAttributes(
                attrs, R.styleable.FloatSeekBarPreference, defStyleAttr, defStyleRes);

        // The ordering of these two statements are important. If we want to set max first, we need
        // to perform the same steps by changing min/max to max/min as following:
        // mMax = a.getInt(...) and setMin(...).
        mMin = a.getFloat(R.styleable.FloatSeekBarPreference_minFloat, 0f);
        float seekbarIncrement = a.getFloat(R.styleable.FloatSeekBarPreference_seekBarIncrementFloat, 1f);
        setMax(a.getFloat(R.styleable.FloatSeekBarPreference_max, 100f) / seekbarIncrement);
        setSeekBarIncrement(seekbarIncrement);
        mAdjustable = a.getBoolean(R.styleable.FloatSeekBarPreference_adjustable, true);
        mShowSeekBarValue = a.getBoolean(R.styleable.FloatSeekBarPreference_showSeekBarValue, false);
        mUpdatesContinuously = a.getBoolean(R.styleable.FloatSeekBarPreference_updatesContinuously,
                false);
        a.recycle();
    }

    public FloatSeekBarPreference(@NonNull Context context, @Nullable AttributeSet attrs,
                                  int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public FloatSeekBarPreference(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, R.attr.seekBarPreferenceStyle);
    }

    public FloatSeekBarPreference(@NonNull Context context) {
        this(context, null);
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        holder.itemView.setOnKeyListener(mSeekBarKeyListener);
        mSeekBar = (SeekBar) holder.findViewById(R.id.seekbar);
        mSeekBarValueTextView = (TextView) holder.findViewById(R.id.seekbar_value);
        if (mShowSeekBarValue) {
            mSeekBarValueTextView.setVisibility(View.VISIBLE);
        } else {
            mSeekBarValueTextView.setVisibility(View.GONE);
            mSeekBarValueTextView = null;
        }

        if (mSeekBar == null) {
            Log.e(TAG, "SeekBar view is null in onBindViewHolder.");
            return;
        }
        mSeekBar.setOnSeekBarChangeListener(mSeekBarChangeListener);
        mSeekBar.setMax(Math.round((mMax - mMin / mSeekBarIncrement)));
        mSeekBar.setProgress(Math.round(((mSeekBarValue - mMin) / mSeekBarIncrement)));
        updateLabelValue(mSeekBarValue);
        mSeekBar.setEnabled(isEnabled());
    }

    @Override
    protected void onSetInitialValue(Object defaultValue) {
        if (defaultValue == null) {
            defaultValue = 0;
        }
        setValue(roundAvoid(getPersistedFloat((Float) defaultValue)));
    }

    @Override
    protected @Nullable Object onGetDefaultValue(@NonNull TypedArray a, int index) {
        return a.getFloat(index, 0);
    }

    /**
     * Gets the lower bound set on the {@link SeekBar}.
     *
     * @return The lower bound set
     */
    public float getMin() {
        return mMin;
    }

    /**
     * Sets the lower bound on the {@link SeekBar}.
     *
     * @param min The lower bound to set
     */
    public void setMin(float min) {
        if (min > mMax) {
            min = mMax;
        }
        if (min != mMin) {
            mMin = min;
            notifyChanged();
        }
    }

    /**
     * Returns the amount of increment change via each arrow key click. This value is derived from
     * user's specified increment value if it's not zero. Otherwise, the default value is picked
     * from the default mKeyProgressIncrement value in {@link android.widget.AbsSeekBar}.
     *
     * @return The amount of increment on the {@link SeekBar} performed after each user's arrow
     * key press
     */
    public final float getSeekBarIncrement() {
        return mSeekBarIncrement;
    }

    /**
     * Sets the increment amount on the {@link SeekBar} for each arrow key press.
     *
     * @param seekBarIncrement The amount to increment or decrement when the user presses an
     *                         arrow key.
     */
    public final void setSeekBarIncrement(float seekBarIncrement) {
        if (seekBarIncrement != mSeekBarIncrement) {
            mSeekBarIncrement = Math.min(mMax - mMin, Math.abs(seekBarIncrement));
            notifyChanged();
        }
    }

    /**
     * Gets the upper bound set on the {@link SeekBar}.
     *
     * @return The upper bound set
     */
    public float getMax() {
        return mMax;
    }

    /**
     * Sets the upper bound on the {@link SeekBar}.
     *
     * @param max The upper bound to set
     */
    public final void setMax(float max) {
        if (max < mMin) {
            max = mMin;
        }
        if (max != mMax) {
            mMax = max;
            notifyChanged();
        }
    }

    /**
     * Gets whether the {@link SeekBar} should respond to the left/right keys.
     *
     * @return Whether the {@link SeekBar} should respond to the left/right keys
     */
    public boolean isAdjustable() {
        return mAdjustable;
    }

    /**
     * Sets whether the {@link SeekBar} should respond to the left/right keys.
     *
     * @param adjustable Whether the {@link SeekBar} should respond to the left/right keys
     */
    public void setAdjustable(boolean adjustable) {
        mAdjustable = adjustable;
    }

    /**
     * Gets whether the {@link FloatSeekBarPreference} should continuously save the {@link SeekBar} value
     * while it is being dragged. Note that when the value is true,
     * {@link Preference.OnPreferenceChangeListener} will be called continuously as well.
     *
     * @return Whether the {@link FloatSeekBarPreference} should continuously save the {@link SeekBar}
     * value while it is being dragged
     * @see #setUpdatesContinuously(boolean)
     */
    public boolean getUpdatesContinuously() {
        return mUpdatesContinuously;
    }

    /**
     * Sets whether the {@link FloatSeekBarPreference} should continuously save the {@link SeekBar} value
     * while it is being dragged.
     *
     * @param updatesContinuously Whether the {@link FloatSeekBarPreference} should continuously save
     *                            the {@link SeekBar} value while it is being dragged
     * @see #getUpdatesContinuously()
     */
    public void setUpdatesContinuously(boolean updatesContinuously) {
        mUpdatesContinuously = updatesContinuously;
    }

    /**
     * Gets whether the current {@link SeekBar} value is displayed to the user.
     *
     * @return Whether the current {@link SeekBar} value is displayed to the user
     * @see #setShowSeekBarValue(boolean)
     */
    public boolean getShowSeekBarValue() {
        return mShowSeekBarValue;
    }

    /**
     * Sets whether the current {@link SeekBar} value is displayed to the user.
     *
     * @param showSeekBarValue Whether the current {@link SeekBar} value is displayed to the user
     * @see #getShowSeekBarValue()
     */
    public void setShowSeekBarValue(boolean showSeekBarValue) {
        mShowSeekBarValue = showSeekBarValue;
        notifyChanged();
    }

    private void setValueInternal(float seekBarValue, boolean notifyChanged) {
        if (seekBarValue < mMin) {
            seekBarValue = mMin;
        }
        if (seekBarValue > mMax) {
            seekBarValue = mMax;
        }

        if (seekBarValue != mSeekBarValue) {
            mSeekBarValue = seekBarValue;
            updateLabelValue(mSeekBarValue);
            persistFloat(seekBarValue);
            if (notifyChanged) {
                notifyChanged();
            }
        }
    }

    /**
     * Gets the current progress of the {@link SeekBar}.
     *
     * @return The current progress of the {@link SeekBar}
     */
    public float getValue() {
        return mSeekBarValue;
    }

    /**
     * Sets the current progress of the {@link SeekBar}.
     *
     * @param seekBarValue The current progress of the {@link SeekBar}
     */
    public void setValue(float seekBarValue) {
        setValueInternal(seekBarValue, true);
    }

    /**
     * Persist the {@link SeekBar}'s SeekBar value if callChangeListener returns true, otherwise
     * set the {@link SeekBar}'s value to the stored value.
     */
    @SuppressWarnings("WeakerAccess")
    void syncValueInternal(@NonNull SeekBar seekBar) {
        float seekBarValue = Math.max(seekBar.getProgress() * mSeekBarIncrement + mMin, mMin);
        if (seekBarValue != mSeekBarValue) {
            if (callChangeListener(seekBarValue)) {
                setValueInternal(seekBarValue, false);
            } else {
                seekBar.setProgress(Math.round((mSeekBarValue - mMin / mSeekBarIncrement)));
                updateLabelValue(mSeekBarValue);
            }
        }
    }

    /**
     * Attempts to update the TextView label that displays the current value.
     *
     * @param value the value to display next to the {@link SeekBar}
     */
    @SuppressWarnings("WeakerAccess")
    void updateLabelValue(float value) {
        if (mSeekBarValueTextView != null) {
            mSeekBarValueTextView.setText(String.valueOf(roundAvoid(value)));
        }
    }

    float roundAvoid(float value) {
        return Float.parseFloat(new DecimalFormat("#.#").format(value));
    }

    @Override
    protected boolean persistFloat(float value) {
        return super.persistFloat(roundAvoid(value));
    }

    @Nullable
    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();
        if (isPersistent()) {
            // No need to save instance state since it's persistent
            return superState;
        }

        // Save the instance state
        final FloatSeekBarPreference.SavedState myState = new FloatSeekBarPreference.SavedState(superState);
        myState.mSeekBarValue = mSeekBarValue;
        myState.mMin = mMin;
        myState.mMax = mMax;
        return myState;
    }

    @Override
    protected void onRestoreInstanceState(@Nullable Parcelable state) {
        if (state == null || !state.getClass().equals(FloatSeekBarPreference.SavedState.class)) {
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState(state);
            return;
        }

        // Restore the instance state
        FloatSeekBarPreference.SavedState myState = (FloatSeekBarPreference.SavedState) state;
        super.onRestoreInstanceState(myState.getSuperState());
        mSeekBarValue = myState.mSeekBarValue;
        mMin = myState.mMin;
        mMax = myState.mMax;
        notifyChanged();
    }

    /**
     * SavedState, a subclass of {@link BaseSavedState}, will store the state of this preference.
     *
     * <p>It is important to always call through to super methods.
     */
    private static class SavedState extends BaseSavedState {
        public static final Parcelable.Creator<FloatSeekBarPreference.SavedState> CREATOR =
                new Parcelable.Creator<FloatSeekBarPreference.SavedState>() {
                    @Override
                    public FloatSeekBarPreference.SavedState createFromParcel(Parcel in) {
                        return new FloatSeekBarPreference.SavedState(in);
                    }

                    @Override
                    public FloatSeekBarPreference.SavedState[] newArray(int size) {
                        return new FloatSeekBarPreference.SavedState[size];
                    }
                };

        float mSeekBarValue;
        float mMin;
        float mMax;

        SavedState(Parcel source) {
            super(source);

            // Restore the click counter
            mSeekBarValue = source.readInt();
            mMin = source.readInt();
            mMax = source.readInt();
        }

        SavedState(Parcelable superState) {
            super(superState);
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);

            // Save the click counter
            dest.writeFloat(mSeekBarValue);
            dest.writeFloat(mMin);
            dest.writeFloat(mMax);
        }
    }
}
