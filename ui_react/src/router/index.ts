import Home from "@/page/Home"
import { createHashRouter } from "react-router"

export default createHashRouter([
    {
        path: "/",
        Component: Home
    }
], {})