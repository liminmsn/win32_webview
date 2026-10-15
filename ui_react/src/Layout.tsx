import { RouterProvider } from "react-router";
import router from "./router";

export default function () {
  return <div className="layout p-1">
    <RouterProvider router={router} />
  </div>
}
